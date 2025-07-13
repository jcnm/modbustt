#include "exporters/in_memory_exporter.h"

namespace modbustt {
namespace exporters {

void InMemoryExporter::configure(const nlohmann::json& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    max_size_ = config.value("max_size", 1000);
}

void InMemoryExporter::export_data(const TelemetryData& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (data_queue_.size() >= max_size_) {
        data_queue_.pop_front();
    }
    data_queue_.push_back(data);
}

std::deque<TelemetryData> InMemoryExporter::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::deque<TelemetryData> flushed_data;
    data_queue_.swap(flushed_data);
    return flushed_data;
}

size_t InMemoryExporter::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_queue_.size();
}

} // namespace exporters
} // namespace modbustt