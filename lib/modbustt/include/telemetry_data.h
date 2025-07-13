#pragma once

#include <string>
#include <map>
#include <chrono>

namespace modbustt {

/**
 * @brief Structure pour stocker les données acquises par un collecteur.
 */
struct TelemetryData {
    std::string collector_id;                     // Identifiant du collecteur
    std::chrono::system_clock::time_point timestamp; // Horodatage de l'acquisition
    std::map<std::string, double> values;         // Nom du point de donnée -> Valeur

    TelemetryData() = default;

    TelemetryData(const std::string& id, const std::map<std::string, double>& data)
        : collector_id(id), timestamp(std::chrono::system_clock::now()), values(data) {
    }
};

} // namespace modbustt