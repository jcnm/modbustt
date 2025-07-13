#pragma once

#include "iexporter.h"
#include <deque>
#include <mutex>

namespace modbustt {
namespace exporters {

class InMemoryExporter : public IExporter {
public:
    void configure(const nlohmann::json& config) override;
    bool connect() override { return true; }
    void disconnect() override {}
    void export_data(const TelemetryData& data) override;
    bool is_connected() const override { return true; }

    // Specific method to retrieve data
    std::deque<TelemetryData> flush();
    size_t size() const;

private:
    std::deque<TelemetryData> data_queue_;
    mutable std::mutex mutex_;
    size_t max_size_ = 1000;

};

} // namespace exporters
} // namespace modbustt
