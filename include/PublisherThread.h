#pragma once

#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include <mqtt/async_client.h>
#include "ModbusData.h"
#include "ConfigManager.h"
#include "AcquisitionThread.h"

/**
 * Thread de publication des données via MQTT
 */
class PublisherThread {
public:
    PublisherThread(const MqttConfig& config);
    ~PublisherThread();
    
    // Gestion du cycle de vie du thread
    bool start();
    void stop();
    void join();
    
    // Gestion des threads d'acquisition
    void addAcquisitionThread(std::shared_ptr<AcquisitionThread> acquisitionThread);
    void removeAcquisitionThread(const std::string& lineId);
    
    // État du thread
    bool isRunning() const { return running_; }
    bool isConnected() const { return connected_; }

private:
    void threadFunction();
    bool connectToMqtt();
    void disconnectFromMqtt();
    void collectAndPublishData();
    std::string aggregateDataToJson();
    
    MqttConfig config_;
    std::unique_ptr<std::thread> thread_;
    std::atomic<bool> running_;
    std::atomic<bool> stopRequested_;
    std::atomic<bool> connected_;
    
    // Client MQTT
    std::unique_ptr<mqtt::async_client> mqttClient_;
    
    // Threads d'acquisition
    std::vector<std::weak_ptr<AcquisitionThread>> acquisitionThreads_;
    // Mutex pour protéger l'accès aux threads d'acquisition;
    mutable std::mutex acquisitionThreadsMutex_;
    
    // Timing
    std::chrono::milliseconds publishPeriod_;
};

