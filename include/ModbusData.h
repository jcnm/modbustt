#pragma once

#include <string>
#include <map>
#include <chrono>

/**
 * Structure pour stocker les données acquises depuis un équipement Modbus
 */
struct ModbusData {
    std::string lineId;                           // Identifiant de la ligne de production
    std::chrono::system_clock::time_point timestamp; // Horodatage de l'acquisition
    std::map<std::string, double> values;         // Nom du point de donnée -> Valeur
    
    // Constructeur par défaut
    ModbusData() = default;
    
    // Constructeur avec paramètres
    ModbusData(const std::string& id, const std::map<std::string, double>& data);
    
    // Conversion en JSON string
    std::string toJson() const;
    
    // Création depuis JSON string
    static ModbusData fromJson(const std::string& json);
};

