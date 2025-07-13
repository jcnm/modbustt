#pragma once

#include "iexporter.h"

namespace modbustt {
namespace exporters {

// This would require a dependency like spdlog or direct use of syslog.h
class SyslogExporter : public IExporter {
public:
    void configure(const nlohmann::json& config) override;
    bool connect() override;
    void disconnect() override;
    void export_data(const TelemetryData& data) override;
    bool is_connected() const override;
private:
    bool connected_ = false;
};

} // namespace exporters
} // namespace modbustt
