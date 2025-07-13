#include "exporters/tcp_exporter.h"
#include "Logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sstream>
#include <fstream>


namespace modbustt {
namespace exporters {

TcpExporter::~TcpExporter() {
    disconnect();
}

void TcpExporter::configure(const nlohmann::json& config) {
    host_ = config.value("host", "127.0.0.1");
    port_ = config.value("port", 5170); // Common port for Fluentd TCP input
}

bool TcpExporter::connect() {
    if (connected_) return true;

    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ < 0) {
        LOG_ERROR("TcpExporter: Could not create socket: " + std::string(strerror(errno)));
        return false;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);

    if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
        LOG_ERROR("TcpExporter: Invalid address or address not supported: " + host_);
        return false;
    }

    if (::connect(sock_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        LOG_ERROR("TcpExporter: Connection failed to " + host_ + ":" + std::to_string(port_) + ": " + std::string(strerror(errno)));
        return false;
    }

    LOG_INFO("TcpExporter: Connected to " + host_ + ":" + std::to_string(port_));
    connected_ = true;
    return true;
}

void TcpExporter::disconnect() {
    if (connected_ && sock_ != -1) {
        close(sock_);
        sock_ = -1;
        connected_ = false;
    }
}

void TcpExporter::export_data(const TelemetryData& data) {
    if (!connected_) return;

    nlohmann::json j;
    j["collector_id"] = data.collector_id;
    auto time_t = std::chrono::system_clock::to_time_t(data.timestamp);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    j["timestamp"] = ss.str();
    j["values"] = data.values;

    std::string payload = j.dump() + "\n"; // Add newline for log parsers

    if (send(sock_, payload.c_str(), payload.length(), 0) < 0) {
        LOG_ERROR("TcpExporter: Failed to send data: " + std::string(strerror(errno)));
        disconnect(); // Assume connection is lost
    }
}

bool TcpExporter::is_connected() const {
    if (!connected_) return false;
    // Optional: more robust check
    char buffer;
    int err = recv(sock_, &buffer, 1, MSG_PEEK | MSG_DONTWAIT);
    if (err == 0) { // Peer has closed the connection
        return false;
    }
    return true;
}

} // namespace exporters
} // namespace modbustt