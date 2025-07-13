#pragma once

#include <thread>
#include <atomic>
#include <memory>
#include <functional>
#include <mqtt/async_client.h>
#include "ConfigManager.h"

/**
 * Callback pour les commandes de reconfiguration
 */
using ReconfigurationCallback = std::function<void(const std::string& command, const std::string& parameters)>;

/**
 * Thread de gestion de la configuration dynamique via MQTT
 */
class ConfigThread : public mqtt::callback {
public:
    ConfigThread(const MqttConfig& config);
    ~ConfigThread();
    
    // Gestion du cycle de vie du thread
    bool start();
    void stop();
    void join();
    
    // Callback pour les reconfigurations
    void setReconfigurationCallback(ReconfigurationCallback callback);
    
    // État du thread
    bool isRunning() const { return running_; }
    bool isConnected() const { return connected_; }

    // Callbacks MQTT (héritées de mqtt::callback)
    void connected(const std::string& cause) override;
    void connection_lost(const std::string& cause) override;
    void message_arrived(mqtt::const_message_ptr msg) override;
    void delivery_complete(mqtt::delivery_token_ptr token) override;

private:
    void threadFunction();
    bool connectToMqtt();
    void disconnectFromMqtt();
    void processCommand(const std::string& jsonCommand);
    
    MqttConfig config_;
    std::unique_ptr<std::thread> thread_;
    std::atomic<bool> running_;
    std::atomic<bool> stopRequested_;
    std::atomic<bool> connected_;
    
    // Client MQTT
    std::unique_ptr<mqtt::async_client> mqttClient_;
    
    // Callback pour les reconfigurations
    ReconfigurationCallback reconfigurationCallback_;
};

