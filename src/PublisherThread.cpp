#include "PublisherThread.h"
#include "Logger.h"
#include <nlohmann/json.hpp>
#include <algorithm>

using json = nlohmann::json;

PublisherThread::PublisherThread(const MqttConfig& config)
    : config_(config)
    , running_(false)
    , stopRequested_(false)
    , connected_(false)
    , publishPeriod_(config.publishFrequencyMs) {
    
    // Créer le client MQTT
    mqttClient_ = std::make_unique<mqtt::async_client>(
        config_.broker + ":" + std::to_string(config_.port),
        config_.clientId
    );
}

PublisherThread::~PublisherThread() {
    stop();
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
    disconnectFromMqtt();
}

bool PublisherThread::start() {
    if (running_) {
        LOG_WARN("Thread de publication déjà démarré");
        return false;
    }
    
    running_ = true;
    stopRequested_ = false;
    
    thread_ = std::make_unique<std::thread>(&PublisherThread::threadFunction, this);
    
    LOG_INFO("Thread de publication démarré");
    return true;
}

void PublisherThread::stop() {
    if (!running_) {
        return;
    }
    
    stopRequested_ = true;
    LOG_INFO("Arrêt demandé pour le thread de publication");
}

void PublisherThread::join() {
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
}

void PublisherThread::addAcquisitionThread(std::shared_ptr<AcquisitionThread> acquisitionThread) {
    std::lock_guard<std::mutex> lock(acquisitionThreadsMutex_);
    acquisitionThreads_.push_back(acquisitionThread);
    LOG_INFO("Thread d'acquisition ajouté au publisher: " + acquisitionThread->getLineId());
}

void PublisherThread::removeAcquisitionThread(const std::string& lineId) {
    std::lock_guard<std::mutex> lock(acquisitionThreadsMutex_);
    
    auto it = std::remove_if(acquisitionThreads_.begin(), acquisitionThreads_.end(),
        [&lineId](const std::weak_ptr<AcquisitionThread>& weak_ptr) {
            auto ptr = weak_ptr.lock();
            return !ptr || ptr->getLineId() == lineId;
        });
    
    if (it != acquisitionThreads_.end()) {
        acquisitionThreads_.erase(it, acquisitionThreads_.end());
        LOG_INFO("Thread d'acquisition retiré du publisher: " + lineId);
    }
}

void PublisherThread::threadFunction() {
    LOG_INFO("Thread de publication en cours d'exécution");
    
    while (!stopRequested_) {
        // Connexion MQTT si nécessaire
        if (!connected_) {
            if (!connectToMqtt()) {
                // Attendre avant de réessayer
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
        }
        
        // Collecter et publier les données
        if (connected_) {
            collectAndPublishData();
        }
        
        // Attendre la prochaine publication
        std::this_thread::sleep_for(publishPeriod_);
    }
    
    disconnectFromMqtt();
    running_ = false;
    
    LOG_INFO("Thread de publication terminé");
}

bool PublisherThread::connectToMqtt() {
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
        
        connected_ = true;
        LOG_INFO("Connexion MQTT établie pour la publication");
        return true;
        
    } catch (const mqtt::exception& e) {
        LOG_ERROR("Erreur de connexion MQTT: " + std::string(e.what()));
        connected_ = false;
        return false;
    }
}

void PublisherThread::disconnectFromMqtt() {
    if (connected_ && mqttClient_) {
        try {
            auto token = mqttClient_->disconnect();
            token->wait();
            LOG_INFO("Déconnexion MQTT effectuée");
        } catch (const mqtt::exception& e) {
            LOG_ERROR("Erreur lors de la déconnexion MQTT: " + std::string(e.what()));
        }
    }
    connected_ = false;
}

void PublisherThread::collectAndPublishData() {
    std::string jsonData = aggregateDataToJson();
    
    if (jsonData.empty()) {
        return; // Pas de données à publier
    }
    
    try {
        mqtt::message_ptr pubmsg = mqtt::make_message(config_.publishTopic, jsonData);
        pubmsg->set_qos(config_.qos);
        
        auto token = mqttClient_->publish(pubmsg);
        token->wait();
        
        LOG_DEBUG("Données publiées sur MQTT: " + std::to_string(jsonData.length()) + " caractères");
        
    } catch (const mqtt::exception& e) {
        LOG_ERROR("Erreur lors de la publication MQTT: " + std::string(e.what()));
        connected_ = false; // Marquer comme déconnecté pour reconnecter
    }
}

std::string PublisherThread::aggregateDataToJson() {
    json aggregatedData;
    aggregatedData["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    json productionLines = json::array();
    
    {
        std::lock_guard<std::mutex> lock(acquisitionThreadsMutex_);
        
        for (auto& weakThread : acquisitionThreads_) {
            auto thread = weakThread.lock();
            if (!thread) {
                continue; // Thread détruit
            }
            
            // Collecter toutes les données disponibles pour ce thread
            std::vector<ModbusData> threadData;
            while (thread->hasData()) {
                threadData.push_back(thread->getData());
            }
            
            if (!threadData.empty()) {
                // Utiliser les données les plus récentes
                const ModbusData& latestData = threadData.back();
                
                json lineData;
                lineData["id"] = latestData.lineId;
                lineData["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                    latestData.timestamp.time_since_epoch()).count();
                lineData["data"] = latestData.values;
                
                productionLines.push_back(lineData);
            }
        }
    }
    
    if (productionLines.empty()) {
        return ""; // Pas de données à publier
    }
    
    aggregatedData["production_lines"] = productionLines;
    
    return aggregatedData.dump();
}

