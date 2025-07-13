#include <iostream>
#include <signal.h>
#include <memory>
#include <vector>
#include <thread>
#include <map>
#include <algorithm>
#include <chrono>
#include <nlohmann/json.hpp>

#include "Logger.h"
#include "ConfigManager.h"
#include "ConfigThread.h"

// --- Utilisation de la nouvelle bibliothèque modbustt ---
#include "modbus_collector.h"
#include "exporters/mqtt_exporter.h" // On supposera que cet exporter existe
#include "exporters/file_exporter.h"

using json = nlohmann::json;

// Variables globales pour la gestion des signaux
static bool g_running = true;
static std::unique_ptr<ConfigThread> g_configThread;
static std::map<std::string, std::shared_ptr<modbustt::ModbusCollector>> g_collectors;

// Gestionnaire de signaux pour arrêt propre
void signalHandler(int signal) {
    LOG_INFO("Signal reçu (" + std::to_string(signal) + "), arrêt en cours...");
    g_running = false;
}

void createCollectors(const std::vector<ProductionLineConfig>& lines, ConfigManager& configManager);

// Fonction pour traiter les commandes de reconfiguration
void handleReconfigurationCommand(const std::string& command, const std::string& parameters, ConfigManager& configManager) {
    try {
        json cmd = json::parse(parameters);
        std::string commandType = cmd["command"];
        
        if (commandType == "pause_line") {
            if (cmd.contains("line_ids")) {
                for (const auto& lineId : cmd["line_ids"]) {
                    auto it = g_collectors.find(lineId.get<std::string>());
                    if (it != g_collectors.end()) {
                        it->second->pause();
                        LOG_INFO("Ligne mise en pause: " + it->first);
                    }
                }
            }
        }
        else if (commandType == "resume_line") {
            if (cmd.contains("line_ids")) {
                for (const auto& lineId : cmd["line_ids"]) {
                    auto it = g_collectors.find(lineId.get<std::string>());
                    if (it != g_collectors.end()) {
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
                auto it = g_collectors.find(lineId);
                if (it != g_collectors.end()) {
                    it->second->setFrequency(cadenceMs);
                    LOG_INFO("Cadence mise à jour pour " + lineId + ": " + std::to_string(cadenceMs) + "ms");
                }
            }
        }
        else if (commandType == "stop_line") {
            if (cmd.contains("line_ids")) {
                for (const auto& lineId : cmd["line_ids"]) {
                    auto it = g_collectors.find(lineId.get<std::string>());
                    if (it != g_collectors.end()) {
                        it->second->stop();
                        it->second->join();
                        g_collectors.erase(it);
                        LOG_INFO("Ligne arrêtée: " + it->first);
                    }
                }
            }
        }
        else if (commandType == "restart_line") {
            if (cmd.contains("line_ids")) {
                for (const auto& lineIdJson : cmd["line_ids"]) {
                    std::string lineId = lineIdJson.get<std::string>();

                    // 1. Vérifier si la ligne existe et l'arrêter si c'est le cas
                    auto it = g_collectors.find(lineId);
                    if (it != g_collectors.end()) {
                        LOG_INFO("Redémarrage: arrêt de la ligne existante " + lineId);
                        it->second->stop();
                        it->second->join();
                        g_collectors.erase(it);
                    }

                    // 2. Recréer la ligne à partir de la configuration initiale
                    // Note: une version plus avancée utiliserait la config fournie dans la commande
                    const auto& allLines = configManager.getProductionLines();
                    auto lineConfigIt = std::find_if(allLines.begin(), allLines.end(), 
                                                     [&lineId](const ProductionLineConfig& cfg){ return cfg.id == lineId; });

                    if (lineConfigIt != allLines.end()) {
                        LOG_INFO("Redémarrage: création d'un nouveau thread pour " + lineId);
                        createCollectors({*lineConfigIt}, configManager);
                    } else {
                        LOG_WARN("Impossible de redémarrer la ligne " + lineId + ": configuration initiale non trouvée.");
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
void createCollectors(const std::vector<ProductionLineConfig>& lines, ConfigManager& configManager) {
    // Créer les exporters une seule fois (ils peuvent être partagés)
    // Pour cet exemple, on crée un exporter MQTT et un exporter Fichier
    auto mqttExporter = std::make_shared<modbustt::exporters::MqttExporter>();
    json mqttConfigJson;
    const auto& mqttConfig = configManager.getMqttConfig();
    mqttConfigJson["broker_address"] = mqttConfig.broker;
    mqttConfigJson["port"] = mqttConfig.port;
    mqttConfigJson["client_id"] = mqttConfig.clientId + "_modbustt";
    mqttConfigJson["topic"] = mqttConfig.publishTopic;
    mqttExporter->configure(mqttConfigJson);
    mqttExporter->connect();

    auto fileExporter = std::make_shared<modbustt::exporters::FileExporter>();
    fileExporter->configure({{"filepath", "telemetry_data.json"}});
    fileExporter->connect();

    for (const auto& line : lines) {
        if (line.enabled) {
            // Traduire la config de l'app en config pour la lib
            modbustt::CollectorConfig collectorConfig;
            collectorConfig.id = line.id;
            collectorConfig.protocol = "tcp"; // Supposons TCP pour l'instant
            collectorConfig.ip_address = line.ip;
            collectorConfig.port = line.port;
            collectorConfig.unit_id = line.unitId;
            collectorConfig.acquisition_frequency_ms = line.acquisitionFrequencyMs;
            // ... mapper les registres ...

            auto collector = std::make_shared<modbustt::ModbusCollector>(collectorConfig); 
            collector->addExporter(mqttExporter); // Publie sur MQTT
            collector->addExporter(fileExporter); // Et écrit dans un fichier
            if (collector->start()) {
                g_collectors[line.id] = collector;
                LOG_INFO("Collecteur démarré pour: " + line.id);
            } else {
                LOG_ERROR("Impossible de démarrer le collecteur pour: " + line.id);
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
    for (auto& pair : g_collectors) {
        pair.second->stop();
    }
    
    // Attendre la fin des threads d'acquisition
    for (auto& pair : g_collectors) {
        pair.second->join();
    }
    g_collectors.clear();
    
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
        // Créer et démarrer le thread de configuration
        g_configThread = std::make_unique<ConfigThread>(mqttConfig);
        // On passe configManager à la callback pour pouvoir recréer les lignes
        g_configThread->setReconfigurationCallback([&configManager](const std::string& cmd, const std::string& params) {
            handleReconfigurationCommand(cmd, params, configManager);
        });
        if (!g_configThread->start()) {
            LOG_ERROR("Impossible de démarrer le thread de configuration");
            return 1;
        }
        
        // Créer et démarrer les collecteurs
        createCollectors(productionLines, configManager);
        
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
