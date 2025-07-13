#include <iostream>
#include <signal.h>
#include <memory>
#include <vector>
#include <thread>
#include <map>
#include <chrono>
#include <nlohmann/json.hpp>

#include "Logger.h"
#include "ConfigManager.h"
#include "AcquisitionThread.h"
#include "PublisherThread.h"
#include "ConfigThread.h"

using json = nlohmann::json;

// Variables globales pour la gestion des signaux
static bool g_running = true;
static std::unique_ptr<PublisherThread> g_publisherThread;
static std::unique_ptr<ConfigThread> g_configThread;
static std::map<std::string, std::shared_ptr<AcquisitionThread>> g_acquisitionThreads;

// Gestionnaire de signaux pour arrêt propre
void signalHandler(int signal) {
    LOG_INFO("Signal reçu (" + std::to_string(signal) + "), arrêt en cours...");
    g_running = false;
}

// Fonction pour traiter les commandes de reconfiguration
void handleReconfigurationCommand(const std::string& command, const std::string& parameters) {
    try {
        json cmd = json::parse(parameters);
        std::string commandType = cmd["command"];
        
        if (commandType == "pause_line") {
            if (cmd.contains("line_ids")) {
                for (const auto& lineId : cmd["line_ids"]) {
                    auto it = g_acquisitionThreads.find(lineId.get<std::string>());
                    if (it != g_acquisitionThreads.end()) {
                        it->second->pause();
                        LOG_INFO("Ligne mise en pause: " + it->first);
                    }
                }
            }
        }
        else if (commandType == "resume_line") {
            if (cmd.contains("line_ids")) {
                for (const auto& lineId : cmd["line_ids"]) {
                    auto it = g_acquisitionThreads.find(lineId.get<std::string>());
                    if (it != g_acquisitionThreads.end()) {
                        it->second->resume();
                        LOG_INFO("Ligne reprise: " + it->first);
                    }
                }
            }
        }
        else if (commandType == "set_cadence") {
            if (cmd.contains("line_id") && cmd.contains("cadence_ms")) {
                std::string lineId = cmd["line_id"].get<std::string>();
                int cadenceMs = cmd["cadence_ms"];
                auto it = g_acquisitionThreads.find(lineId);
                if (it != g_acquisitionThreads.end()) {
                    it->second->setFrequency(cadenceMs);
                    LOG_INFO("Cadence mise à jour pour " + lineId + ": " + std::to_string(cadenceMs) + "ms");
                }
            }
        }
        else if (commandType == "stop_line") {
            if (cmd.contains("line_ids")) {
                for (const auto& lineId : cmd["line_ids"]) {
                    auto it = g_acquisitionThreads.find(lineId.get<std::string>());
                    if (it != g_acquisitionThreads.end()) {
                        it->second->stop();
                        it->second->join();
                        g_publisherThread->removeAcquisitionThread(it->first);
                        g_acquisitionThreads.erase(it);
                        LOG_INFO("Ligne arrêtée: " + it->first);
                    }
                }
            }
        }
        else {
            LOG_WARN("Commande non reconnue: " + commandType);
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Erreur lors du traitement de la commande de reconfiguration: " + std::string(e.what()));
    }
}

// Fonction pour créer et démarrer les threads d'acquisition
void createAcquisitionThreads(const std::vector<ProductionLineConfig>& lines) {
    for (const auto& line : lines) {
        if (line.enabled) {
            auto acquisitionThread = std::make_shared<AcquisitionThread>(line);
            if (acquisitionThread->start()) {
                g_acquisitionThreads[line.id] = acquisitionThread;
                g_publisherThread->addAcquisitionThread(acquisitionThread);
                LOG_INFO("Thread d'acquisition créé et démarré pour: " + line.id);
            } else {
                LOG_ERROR("Impossible de démarrer le thread d'acquisition pour: " + line.id);
            }
        } else {
            LOG_INFO("Ligne désactivée, thread non créé: " + line.id);
        }
    }
}

// Fonction pour arrêter tous les threads
void stopAllThreads() {
    LOG_INFO("Arrêt de tous les threads...");
    
    // Arrêter les threads d'acquisition
    for (auto& pair : g_acquisitionThreads) {
        pair.second->stop();
    }
    
    // Attendre la fin des threads d'acquisition
    for (auto& pair : g_acquisitionThreads) {
        pair.second->join();
    }
    g_acquisitionThreads.clear();
    
    // Arrêter le thread de publication
    if (g_publisherThread) {
        g_publisherThread->stop();
        g_publisherThread->join();
    }
    
    // Arrêter le thread de configuration
    if (g_configThread) {
        g_configThread->stop();
        g_configThread->join();
    }
    
    LOG_INFO("Tous les threads arrêtés");
}

int main(int argc, char* argv[]) {
    // Configuration du gestionnaire de signaux
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Initialisation du logger
    Logger::getInstance().setLogLevel(LogLevel::INFO);
    Logger::getInstance().setLogFile("supervision.log");
    
    LOG_INFO("=== Démarrage du Système de Supervision ===");
    
    // Déterminer le fichier de configuration
    std::string configFile = "config/config.yaml";
    if (argc > 1) {
        configFile = argv[1];
    }
    
    // Chargement de la configuration
    ConfigManager configManager;
    if (!configManager.loadConfig(configFile)) {
        LOG_ERROR("Impossible de charger la configuration depuis: " + configFile);
        return 1;
    }
    
    const auto& mqttConfig = configManager.getMqttConfig();
    const auto& productionLines = configManager.getProductionLines();
    
    LOG_INFO("Configuration chargée: " + std::to_string(productionLines.size()) + " lignes de production");
    
    try {
        // Créer et démarrer le thread de publication
        g_publisherThread = std::make_unique<PublisherThread>(mqttConfig);
        if (!g_publisherThread->start()) {
            LOG_ERROR("Impossible de démarrer le thread de publication");
            return 1;
        }
        
        // Créer et démarrer le thread de configuration
        g_configThread = std::make_unique<ConfigThread>(mqttConfig);
        g_configThread->setReconfigurationCallback(handleReconfigurationCommand);
        if (!g_configThread->start()) {
            LOG_ERROR("Impossible de démarrer le thread de configuration");
            return 1;
        }
        
        // Créer et démarrer les threads d'acquisition
        createAcquisitionThreads(productionLines);
        
        LOG_INFO("Système de supervision démarré avec succès");
        
        // Boucle principale - surveillance des changements de configuration
        auto lastConfigCheck = std::chrono::steady_clock::now();
        const auto configCheckInterval = std::chrono::seconds(10);
        
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // Vérifier périodiquement les changements de configuration
            auto now = std::chrono::steady_clock::now();
            if (now - lastConfigCheck >= configCheckInterval) {
                if (configManager.hasConfigChanged()) {
                    LOG_INFO("Changement de configuration détecté, rechargement...");
                    if (configManager.reloadConfig()) {
                        configManager.markConfigAsRead();
                        // Note: Pour une implémentation complète, il faudrait ici
                        // reconfigurer les threads existants ou en créer de nouveaux
                        LOG_INFO("Configuration rechargée avec succès");
                    } else {
                        LOG_ERROR("Erreur lors du rechargement de la configuration");
                    }
                }
                lastConfigCheck = now;
            }
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Erreur fatale: " + std::string(e.what()));
        stopAllThreads();
        return 1;
    }
    
    // Arrêt propre
    stopAllThreads();
    
    LOG_INFO("=== Arrêt du Système de Supervision ===");
    return 0;
}
