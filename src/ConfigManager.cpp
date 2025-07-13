#include "ConfigManager.h"
#include "Logger.h"
#include <fstream>
#include <sys/stat.h>

ConfigManager::ConfigManager() : lastModificationTime_(0) {
}

bool ConfigManager::loadConfig(const std::string& configFile) {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    try {
        configFilePath_ = configFile;
        YAML::Node config = YAML::LoadFile(configFile);
        
        // Parse MQTT configuration
        if (config["mqtt"]) {
            parseMqttConfig(config["mqtt"]);
        } else {
            LOG_ERROR("Configuration MQTT manquante dans le fichier de configuration");
            return false;
        }
        
        // Parse production lines configuration
        if (config["production_lines"]) {
            parseProductionLines(config["production_lines"]);
        } else {
            LOG_WARN("Aucune ligne de production configurée");
        }
        
        // Mettre à jour le timestamp de dernière modification
        lastModificationTime_ = getFileModificationTime(configFile);
        
        LOG_INFO("Configuration chargée avec succès depuis: " + configFile);
        return true;
        
    } catch (const YAML::Exception& e) {
        LOG_ERROR("Erreur lors du parsing YAML: " + std::string(e.what()));
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("Erreur lors du chargement de la configuration: " + std::string(e.what()));
        return false;
    }
}

bool ConfigManager::reloadConfig() {
    return loadConfig(configFilePath_);
}

const std::vector<ProductionLineConfig>& ConfigManager::getProductionLines() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return productionLines_;
}

const MqttConfig& ConfigManager::getMqttConfig() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return mqttConfig_;
}

bool ConfigManager::updateLineConfig(const std::string& lineId, const ProductionLineConfig& newConfig) {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    for (auto& line : productionLines_) {
        if (line.id == lineId) {
            line = newConfig;
            LOG_INFO("Configuration mise à jour pour la ligne: " + lineId);
            return true;
        }
    }
    
    LOG_WARN("Ligne non trouvée pour mise à jour: " + lineId);
    return false;
}

bool ConfigManager::enableLine(const std::string& lineId, bool enable) {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    for (auto& line : productionLines_) {
        if (line.id == lineId) {
            line.enabled = enable;
            LOG_INFO("Ligne " + lineId + (enable ? " activée" : " désactivée"));
            return true;
        }
    }
    
    LOG_WARN("Ligne non trouvée: " + lineId);
    return false;
}

bool ConfigManager::setLineFrequency(const std::string& lineId, int frequencyMs) {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    for (auto& line : productionLines_) {
        if (line.id == lineId) {
            line.acquisitionFrequencyMs = frequencyMs;
            LOG_INFO("Fréquence mise à jour pour la ligne " + lineId + ": " + std::to_string(frequencyMs) + "ms");
            return true;
        }
    }
    
    LOG_WARN("Ligne non trouvée: " + lineId);
    return false;
}

bool ConfigManager::hasConfigChanged() const {
    std::time_t currentModTime = getFileModificationTime(configFilePath_);
    return currentModTime > lastModificationTime_;
}

void ConfigManager::markConfigAsRead() {
    lastModificationTime_ = getFileModificationTime(configFilePath_);
}

void ConfigManager::parseProductionLines(const YAML::Node& node) {
    productionLines_.clear();
    
    for (const auto& lineNode : node) {
        ProductionLineConfig line;
        
        line.id = lineNode["id"].as<std::string>();
        line.ip = lineNode["ip"].as<std::string>();
        line.port = lineNode["port"].as<int>(502);
        line.unitId = lineNode["unit_id"].as<int>(1);
        line.acquisitionFrequencyMs = lineNode["acquisition_frequency_ms"].as<int>(200);
        line.enabled = lineNode["enabled"].as<bool>(true);
        
        // Parse registers
        if (lineNode["registers"]) {
            for (const auto& regNode : lineNode["registers"]) {
                ModbusRegister reg;
                reg.address = regNode["address"].as<int>();
                reg.name = regNode["name"].as<std::string>();
                reg.type = regNode["type"].as<std::string>();
                reg.scale = regNode["scale"].as<double>(1.0);
                reg.offset = regNode["offset"].as<double>(0.0);
                
                line.registers.push_back(reg);
            }
        }
        
        productionLines_.push_back(line);
        LOG_INFO("Ligne de production configurée: " + line.id + " (" + line.ip + ":" + std::to_string(line.port) + ")");
    }
}

void ConfigManager::parseMqttConfig(const YAML::Node& node) {
    mqttConfig_.broker = node["broker"].as<std::string>();
    mqttConfig_.port = node["port"].as<int>(1883);
    mqttConfig_.clientId = node["client_id"].as<std::string>();
    mqttConfig_.username = node["username"].as<std::string>("");
    mqttConfig_.password = node["password"].as<std::string>("");
    mqttConfig_.publishTopic = node["publish_topic"].as<std::string>("supervision/data");
    mqttConfig_.commandTopic = node["command_topic"].as<std::string>("supervision/commands");
    mqttConfig_.publishFrequencyMs = node["publish_frequency_ms"].as<int>(800);
    mqttConfig_.qos = node["qos"].as<int>(1);
    
    LOG_INFO("Configuration MQTT: " + mqttConfig_.broker + ":" + std::to_string(mqttConfig_.port));
}

std::time_t ConfigManager::getFileModificationTime(const std::string& filepath) const {
    struct stat fileInfo;
    if (stat(filepath.c_str(), &fileInfo) == 0) {
        return fileInfo.st_mtime;
    }
    return 0;
}

