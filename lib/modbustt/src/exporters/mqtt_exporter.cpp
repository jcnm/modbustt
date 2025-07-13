#include "exporters/mqtt_exporter.h"
#include "Logger.h"
#include <iomanip>

namespace modbustt {
namespace exporters {

MqttExporter::MqttExporter() : qos_(1), connected_(false) {}

MqttExporter::~MqttExporter() {
    disconnect();
}

void MqttExporter::configure(const nlohmann::json& config) {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    std::string broker = config.value("broker_address", "tcp://localhost");
    int port = config.value("port", 1883);
    server_uri_ = broker + ":" + std::to_string(port);
    client_id_ = config.value("client_id", "modbustt_exporter");
    topic_ = config.value("topic", "modbustt/data");
    qos_ = config.value("qos", 1);

    conn_opts_.set_keep_alive_interval(20);
    conn_opts_.set_clean_session(true);

    if (config.contains("username") && !config["username"].get<std::string>().empty()) {
        conn_opts_.set_user_name(config["username"].get<std::string>());
        conn_opts_.set_password(config.value("password", ""));
    }
}

bool MqttExporter::connect() {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    if (connected_) return true;

    client_ = std::make_unique<mqtt::async_client>(server_uri_, client_id_);
    client_->set_callback(*this);

    try {
        LOG_INFO("MqttExporter: Connecting to " + server_uri_);
        auto token = client_->connect(conn_opts_);
        token->wait_for(std::chrono::seconds(10));
        if (!token->is_complete() || token->get_return_code() != 0) {
             LOG_ERROR("MqttExporter: Connection failed.");
             connected_ = false;
             return false;
        }
    } catch (const mqtt::exception& e) {
        LOG_ERROR("MqttExporter: Connection error: " + std::string(e.what()));
        connected_ = false;
        return false;
    }
    return connected_;
}

void MqttExporter::disconnect() {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    if (!connected_ || !client_) return;
    try {
        client_->disconnect()->wait();
    } catch (const mqtt::exception& e) {
        LOG_ERROR("MqttExporter: Disconnect error: " + std::string(e.what()));
    }
    connected_ = false;
}

void MqttExporter::export_data(const TelemetryData& data) {
    if (!connected_) return;

    nlohmann::json j;
    j["collector_id"] = data.collector_id;
    auto time_t = std::chrono::system_clock::to_time_t(data.timestamp);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    j["timestamp"] = ss.str();
    j["values"] = data.values;

    try {
        client_->publish(topic_, j.dump(), qos_, false);
    } catch (const mqtt::exception& e) {
        LOG_ERROR("MqttExporter: Failed to publish message: " + std::string(e.what()));
    }
}

void MqttExporter::connected(const std::string& cause) {
    LOG_INFO("MqttExporter: Connection successful.");
    connected_ = true;
}

void MqttExporter::connection_lost(const std::string& cause) {
    LOG_WARN("MqttExporter: Connection lost: " + cause);
    connected_ = false;
    // Note: A robust implementation would attempt to reconnect here.
}

void MqttExporter::delivery_complete(mqtt::delivery_token_ptr token) {
    // Optional: log that the message was delivered
}

} // namespace exporters
} // namespace modbustt