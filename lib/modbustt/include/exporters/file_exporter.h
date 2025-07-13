#pragma once

#include "iexporter.h"
#include <fstream>
#include <mutex>

namespace modbustt {
namespace exporters {

class FileExporter : public IExporter {
public:
    ~FileExporter() override;

    void configure(const nlohmann::json& config) override;
    bool connect() override;
    void disconnect() override;
    void export_data(const TelemetryData& data) override;
    bool is_connected() const override { return file_stream_.is_open(); }

private:
    std::ofstream file_stream_;
    std::string filepath_;
    mutable std::mutex mutex_;
};

} // namespace exporters
} // namespace modbustt