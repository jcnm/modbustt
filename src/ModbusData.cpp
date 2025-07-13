#include "ModbusData.h"
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

ModbusData::ModbusData(const std::string& id, const std::map<std::string, double>& data)
    : lineId(id), timestamp(std::chrono::system_clock::now()), values(data) {
}

std::string ModbusData::toJson() const {
    json j;
    j["line_id"] = lineId;
    
    // Conversion du timestamp en millisecondes depuis epoch
    auto time_since_epoch = timestamp.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
    j["timestamp"] = millis;
    
    j["data"] = values;
    
    return j.dump();
}

ModbusData ModbusData::fromJson(const std::string& jsonStr) {
    json j = json::parse(jsonStr);
    
    ModbusData data;
    data.lineId = j["line_id"];
    
    // Conversion du timestamp depuis millisecondes
    long long millis = j["timestamp"];
    data.timestamp = std::chrono::system_clock::time_point(std::chrono::milliseconds(millis));
    
    data.values = j["data"];
    
    return data;
}

