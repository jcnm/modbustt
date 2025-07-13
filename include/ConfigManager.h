#pragma once

#include <string>
#include <vector>
#include <map>
#include <yaml-cpp/yaml.h>
#include <mutex>

/**
 * Structure pour la configuration d'un registre Modbus
 */
struct ModbusRegister {
    int address;
    std::string name;
    std::string type; // "holding", "input", "coil", "discrete"
    double scale = 1.0;
    double offset = 0.0;
};

/**
 * Structure pour la configuration d'une ligne de production
 */
struct ProductionLineConfig {
    std::string id;
    std::string ip;
    int port = 502;
    int unitId = 1;
    int acquisitionFrequencyMs = 200;
    std::vector<ModbusRegister> registers;
    bool enabled = true;
};

/**
 * Structure pour la configuration MQTT
 */
struct MqttConfig {
    std::string broker;
    int port = 1883;
    std::string clientId;
    std::string username;
    std::string password;
    std::string publishTopic = "supervision/data";
    std::string commandTopic = "supervision/commands";
    int publishFrequencyMs = 800;
    int qos = 1;
};

/**
 * Gestionnaire de configuration
 */
class ConfigManager {
public:
    ConfigManager();
    
    bool loadConfig(const std::string& configFile);
    bool reloadConfig();
    
    const std::vector<ProductionLineConfig>& getProductionLines() const;
    const MqttConfig& getMqttConfig() const;
    
    // Méthodes pour la reconfiguration dynamique
    bool updateLineConfig(const std::string& lineId, const ProductionLineConfig& newConfig);
    bool enableLine(const std::string& lineId, bool enable);
    bool setLineFrequency(const std::string& lineId, int frequencyMs);
    
    // Surveillance des changements de fichier
    bool hasConfigChanged() const;
    void markConfigAsRead();

private:
    std::string configFilePath_;
    std::vector<ProductionLineConfig> productionLines_;
    MqttConfig mqttConfig_;
    mutable std::mutex configMutex_;
    
    // Pour la surveillance des changements
    std::time_t lastModificationTime_;
    
    void parseProductionLines(const YAML::Node& node);
    void parseMqttConfig(const YAML::Node& node);
    std::time_t getFileModificationTime(const std::string& filepath) const;
};

