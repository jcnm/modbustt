#include "exporters/file_exporter.h"
#include "Logger.h"
#include <iomanip>
#include <fstream>
#include <sstream>

using namespace std;

namespace modbustt {
namespace exporters {

FileExporter::~FileExporter() {
    disconnect();
}

void FileExporter::configure(const nlohmann::json& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    filepath_ = config.value("filepath", "modbustt_output.jsonl");
}

bool FileExporter::connect() {
    std::lock_guard<std::mutex> lock(mutex_);
    file_stream_.open(filepath_, std::ios::app);
    if (!file_stream_.is_open()) {
        LOG_ERROR("FileExporter: Could not open file " + filepath_);
        return false;
    }
    LOG_INFO("FileExporter: Log file opened at " + filepath_);
    return true;
}

void FileExporter::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
}

void FileExporter::export_data(const TelemetryData& data) {
    nlohmann::json j;
    j["collector_id"] = data.collector_id;
    
    auto time_t = std::chrono::system_clock::to_time_t(data.timestamp);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    j["timestamp"] = ss.str();

    j["values"] = data.values;

    std::lock_guard<std::mutex> lock(mutex_);
    if (file_stream_.is_open()) {
        file_stream_ << j.dump() << std::endl; // JSON Lines format is great for logs
    }
}

} // namespace exporters
} // namespace modbustt