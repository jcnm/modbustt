#pragma once

#include "../telemetry_data.h"
#include <nlohmann/json.hpp>
#include <memory>

namespace modbustt {
namespace exporters {

/**
 * @brief Interface abstraite pour tous les exporters de données.
 */
class IExporter {
public:
    virtual ~IExporter() = default;

    virtual void configure(const nlohmann::json& config) = 0;
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual void export_data(const TelemetryData& data) = 0;
    virtual bool is_connected() const = 0;
};

} // namespace exporters
} // namespace modbustt