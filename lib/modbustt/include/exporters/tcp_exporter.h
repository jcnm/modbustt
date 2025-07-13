#pragma once

#include "iexporter.h"

namespace modbustt {
namespace exporters {

// This would implement a simple TCP client to send data to a listening server
class TcpExporter : public IExporter {
public:
    ~TcpExporter() override;

    void configure(const nlohmann::json& config) override;
    bool connect() override;
    void disconnect() override;
    void export_data(const TelemetryData& data) override;
    bool is_connected() const override;

private:
    // Would use OS-specific sockets or a library like Boost.Asio
    int sock_ = -1;
    bool connected_ = false;
    std::string host_ = "localhost";
    int port_ = 5170; // Default port for TCP exporter


};

} // namespace exporters
} // namespace modbustt