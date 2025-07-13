#include "exporters/syslog_exporter.h"
#include "Logger.h"
#include <syslog.h>
#include <sstream>

namespace modbustt {
namespace exporters {

void SyslogExporter::configure(const nlohmann::json& config) {
    ident_ = config.value("ident", "modbustt");
}

bool SyslogExporter::connect() {
    if (connected_) return true;
    // LOG_PID: include PID with each message
    // LOG_CONS: write to console if error sending to syslogd
    // LOG_USER: user-level messages
    openlog(ident_.c_str(), LOG_PID | LOG_CONS, LOG_USER);
    // LOG_INFO("SyslogExporter: Connected to syslog daemon with ident '" + ident_ + "'");
    connected_ = true;
    return true;
}

void SyslogExporter::disconnect() {
    if (connected_) {
        closelog();
        connected_ = false;
    }
}

void SyslogExporter::export_data(const TelemetryData& data) {
    if (!connected_) return;

    std::stringstream ss;
    ss << "collector=" << data.collector_id;
    for (const auto& pair : data.values) {
        ss << " " << pair.first << "=" << pair.second;
    }

    // LOG_INFO is the priority level for the message
    syslog(LOG_INFO, "%s", ss.str().c_str());
}

bool SyslogExporter::is_connected() const {
    return connected_;
}

} // namespace exporters
} // namespace modbustt