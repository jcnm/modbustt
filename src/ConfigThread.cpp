#include "ConfigThread.h"
#include "Logger.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

ConfigThread::ConfigThread(const MqttConfig& config)
    : config_(config)
    , running_(false)
    , stopRequested_(false)
    , connected_(false) {
    
    // Créer le client MQTT
    mqttClient_ = std::make_unique<mqtt::async_client>(
        config_.broker + ":" + std::to_string(config_.port),
        config_.clientId + "_config"
    );
    
    // Définir ce callback comme gestionnaire des messages
    mqttClient_->set_callback(*this);
}

ConfigThread::~ConfigThread() {
    stop();
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
    disconnectFromMqtt();
}

bool ConfigThread::start() {
    if (running_) {
        LOG_WARN("Thread de configuration déjà démarré");
        return false;
    }
    
    running_ = true;
    stopRequested_ = false;
    
    thread_ = std::make_unique<std::thread>(&ConfigThread::threadFunction, this);
    
    LOG_INFO("Thread de configuration démarré");
    return true;
}

void ConfigThread::stop() {
    if (!running_) {
        return;
    }
    
    stopRequested_ = true;
    LOG_INFO("Arrêt demandé pour le thread de configuration");
}

void ConfigThread::join() {
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
}

void ConfigThread::setReconfigurationCallback(ReconfigurationCallback callback) {
    reconfigurationCallback_ = callback;
}

void ConfigThread::connected(const std::string& cause) {
    LOG_INFO("Connexion MQTT établie pour la configuration: " + cause);
    connected_ = true;
    
    // S'abonner au topic de commandes
    try {
        mqttClient_->subscribe(config_.commandTopic, config_.qos);
        LOG_INFO("Abonnement au topic de commandes: " + config_.commandTopic);
    } catch (const mqtt::exception& e) {
        LOG_ERROR("Erreur lors de l'abonnement MQTT: " + std::string(e.what()));
    }
}

void ConfigThread::connection_lost(const std::string& cause) {
    LOG_WARN("Connexion MQTT perdue pour la configuration: " + cause);
    connected_ = false;
}

void ConfigThread::message_arrived(mqtt::const_message_ptr msg) {
    std::string topic = msg->get_topic();
    std::string payload = msg->to_string();
    
    LOG_INFO("Message reçu sur " + topic + ": " + payload);
    
    if (topic == config_.commandTopic) {
        processCommand(payload);
    }
}

void ConfigThread::delivery_complete(mqtt::delivery_token_ptr token) {
    // Pas utilisé pour la réception de messages
}

void ConfigThread::threadFunction() {
    LOG_INFO("Thread de configuration en cours d'exécution");
    
    while (!stopRequested_) {
        // Connexion MQTT si nécessaire
        if (!connected_) {
            if (!connectToMqtt()) {
                // Attendre avant de réessayer
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
        }
        
        // Le thread reste en vie pour recevoir les messages MQTT
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    disconnectFromMqtt();
    running_ = false;
    
    LOG_INFO("Thread de configuration terminé");
}

bool ConfigThread::connectToMqtt() {
    try {
        mqtt::connect_options connOpts;
        connOpts.set_keep_alive_interval(20);
        connOpts.set_clean_session(true);
        
        if (!config_.username.empty()) {
            connOpts.set_user_name(config_.username);
            connOpts.set_password(config_.password);
        }
        
        auto token = mqttClient_->connect(connOpts);
        token->wait();
        
        // La connexion sera confirmée dans le callback connected()
        return true;
        
    } catch (const mqtt::exception& e) {
        LOG_ERROR("Erreur de connexion MQTT pour la configuration: " + std::string(e.what()));
        connected_ = false;
        return false;
    }
}

void ConfigThread::disconnectFromMqtt() {
    if (connected_ && mqttClient_) {
        try {
            auto token = mqttClient_->disconnect();
            token->wait();
            LOG_INFO("Déconnexion MQTT effectuée pour la configuration");
        } catch (const mqtt::exception& e) {
            LOG_ERROR("Erreur lors de la déconnexion MQTT: " + std::string(e.what()));
        }
    }
    connected_ = false;
}

void ConfigThread::processCommand(const std::string& jsonCommand) {
    try {
        json cmd = json::parse(jsonCommand);
        
        if (!cmd.contains("command")) {
            LOG_ERROR("Commande malformée: champ 'command' manquant");
            return;
        }
        
        std::string command = cmd["command"];
        LOG_INFO("Traitement de la commande: " + command);
        
        if (reconfigurationCallback_) {
            reconfigurationCallback_(command, jsonCommand);
        } else {
            LOG_WARN("Aucun callback de reconfiguration défini");
        }
        
    } catch (const json::parse_error& e) {
        LOG_ERROR("Erreur de parsing JSON de la commande: " + std::string(e.what()));
    } catch (const std::exception& e) {
        LOG_ERROR("Erreur lors du traitement de la commande: " + std::string(e.what()));
    }
}

