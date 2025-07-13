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

    /**
     * @brief Configure l'exporter avec les paramètres donnés.
     * @param config Configuration sous forme de JSON.
     */
    virtual void configure(const nlohmann::json& config) = 0;
    /**
     * @brief Établit une connexion avec l'exporter.
     * @return true si la connexion est réussie, false sinon.
     */
    virtual bool connect() = 0;
    /**
     * @brief Déconnecte l'exporter.
     */
    virtual void disconnect() = 0;
    /**
     * @brief Exporte les données de télémétrie.
     * @param data Les données de télémétrie à exporter.
     */
    virtual void export_data(const TelemetryData& data) = 0;
    /**
     * @brief Vérifie si l'exporter est connecté.
     * @return true si l'exporter est connecté, false sinon.
     */
    virtual bool is_connected() const = 0;

};

} // namespace exporters
} // namespace modbustt