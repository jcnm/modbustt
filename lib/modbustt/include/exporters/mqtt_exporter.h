#pragma once

#include "iexporter.h"
#include <mqtt/async_client.h>
#include <atomic>
#include <mutex>

namespace modbustt {
namespace exporters {

class MqttExporter : public IExporter, public virtual mqtt::callback {
public:
    MqttExporter();
    ~MqttExporter() override;

    void configure(const nlohmann::json& config) override;
    bool connect() override;
    void disconnect() override;
    void export_data(const TelemetryData& data) override;
    bool is_connected() const override { return connected_; }

    // MQTT Callbacks
    void connected(const std::string& cause) override;
    void connection_lost(const std::string& cause) override;
    void delivery_complete(mqtt::delivery_token_ptr token) override;

private:
    std::unique_ptr<mqtt::async_client> client_;
    std::string server_uri_;
    std::string client_id_;
    std::string topic_;
    int qos_ = 1;
    std::string username_;
    std::string password_;
    mqtt::connect_options conn_opts_;
    std::atomic<bool> connected_{false};
    std::mutex connection_mutex_;
};

} // namespace exporters
} // namespace modbustt