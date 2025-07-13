#include "modbus_collector.h"
#include "Logger.h" // On suppose que le logger est accessible
#include <chrono>

namespace modbustt {

ModbusCollector::ModbusCollector(const CollectorConfig& config)
    : config_(config), acquisitionPeriod_(config.acquisition_frequency_ms) {}

ModbusCollector::~ModbusCollector() {
    stop();
    join();
    disconnectFromModbus();
}

bool ModbusCollector::start() {
    if (running_) return false;
    running_ = true;
    stopRequested_ = false;
    paused_ = false;
    thread_ = std::make_unique<std::thread>(&ModbusCollector::threadFunction, this);
    LOG_INFO("Collector started for: " + config_.id);
    return true;
}

void ModbusCollector::stop() {
    if (!running_) return;
    {
        std::lock_guard<std::mutex> lock(controlMutex_);
        controlQueue_.push({CollectorCommand::STOP});
    }
    controlCondition_.notify_all();
    LOG_INFO("Stop requested for collector: " + config_.id);
}

void ModbusCollector::join() {
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
}

void ModbusCollector::addExporter(std::shared_ptr<exporters::IExporter> exporter) {
    std::lock_guard<std::mutex> lock(controlMutex_);
    exporters_.push_back(exporter);
}

void ModbusCollector::pause() {
    std::lock_guard<std::mutex> lock(controlMutex_);
    controlQueue_.push({CollectorCommand::PAUSE});
    controlCondition_.notify_one();
}

void ModbusCollector::resume() {
    std::lock_guard<std::mutex> lock(controlMutex_);
    controlQueue_.push({CollectorCommand::RESUME});
    controlCondition_.notify_one();
}

void ModbusCollector::setFrequency(int frequencyMs) {
    std::lock_guard<std::mutex> lock(controlMutex_);
    controlQueue_.push({CollectorCommand::SET_FREQUENCY, frequencyMs});
    controlCondition_.notify_one();
}

void ModbusCollector::threadFunction() {
    LOG_INFO("Collector thread running for: " + config_.id);
    while (!stopRequested_) {
        processControlMessages();
        if (stopRequested_) break;

        if (paused_) {
            std::unique_lock<std::mutex> lock(controlMutex_);
            controlCondition_.wait(lock, [this] { return !paused_ || stopRequested_; });
            continue;
        }

        if (!connected_) {
            if (!connectToModbus()) {
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
        }

        if (connected_) {
            readRegisters();
        }

        // Attente contrôlée par la condition variable pour un arrêt réactif
        std::unique_lock<std::mutex> lock(controlMutex_);
        controlCondition_.wait_for(lock, acquisitionPeriod_, [this] { return stopRequested_.load(); });
    }
    disconnectFromModbus();
    running_ = false;
    LOG_INFO("Collector thread finished for: " + config_.id);
}

bool ModbusCollector::connectToModbus() {
    disconnectFromModbus();

    if (config_.protocol == "tcp") {
        modbusContext_ = modbus_new_tcp(config_.ip_address.c_str(), config_.port);
    } else if (config_.protocol == "rtu") {
        const auto& rtu = config_.rtu_settings;
        modbusContext_ = modbus_new_rtu(rtu.serial_port.c_str(), rtu.baud_rate, rtu.parity, rtu.data_bits, rtu.stop_bits);
    } else {
        LOG_ERROR("Unsupported protocol: " + config_.protocol + " for " + config_.id);
        return false;
    }

    if (!modbusContext_) {
        LOG_ERROR("Failed to create Modbus context for " + config_.id + ": " + modbus_strerror(errno));
        return false;
    }

    modbus_set_slave(modbusContext_, config_.unit_id);
    modbus_set_response_timeout(modbusContext_, 1, 0);

    if (modbus_connect(modbusContext_) == -1) {
        LOG_ERROR("Modbus connection failed for " + config_.id + ": " + modbus_strerror(errno));
        modbus_free(modbusContext_);
        modbusContext_ = nullptr;
        return false;
    }

    connected_ = true;
    LOG_INFO("Modbus connection established for " + config_.id);
    return true;
}

void ModbusCollector::disconnectFromModbus() {
    if (modbusContext_) {
        modbus_close(modbusContext_);
        modbus_free(modbusContext_);
        modbusContext_ = nullptr;
    }
    connected_ = false;
}

bool ModbusCollector::readRegisters() {
    if (!connected_ || !modbusContext_) return false;

    std::map<std::string, double> values;
    bool success = true;

    for (const auto& reg : config_.registers) {
        uint16_t rawValue[1] = {0};
        int result = -1;

        if (reg.type == "holding") {
            result = modbus_read_registers(modbusContext_, reg.address - 1, 1, rawValue);
        } else if (reg.type == "input") {
            result = modbus_read_input_registers(modbusContext_, reg.address - 1, 1, rawValue);
        } else if (reg.type == "coil") {
            uint8_t bitValue[1];
            result = modbus_read_bits(modbusContext_, reg.address - 1, 1, bitValue);
            if (result != -1) rawValue[0] = bitValue[0];
        } else if (reg.type == "discrete") {
            uint8_t bitValue[1];
            result = modbus_read_input_bits(modbusContext_, reg.address - 1, 1, bitValue);
            if (result != -1) rawValue[0] = bitValue[0];
        }

        if (result == -1) {
            LOG_ERROR("Error reading register " + std::to_string(reg.address) + " for " + config_.id + ": " + modbus_strerror(errno));
            success = false;
            connected_ = false; // Assume connection is lost on error
            break;
        } else {
            double scaledValue = (static_cast<double>(rawValue[0]) * reg.scale) + reg.offset;
            values[reg.name] = scaledValue;
        }
    }

    if (success && !values.empty()) {
        // Créer un objet TelemetryData et l'exporter
        TelemetryData data(config_.id, values);
        // TODO: Ajouter alternative pour ne pas bloquer le thread ET ne pas perdre de données si aucune exporter est configurée ou est déconnectée
        // La lib sert à ingérer des données, pas à les stocker dans le cadre du développement du mbserve, 
        // il faudra un exporter en mémoire (un exporter qui stocke les données dans une structure modbus server, 
        // modbustt::exporters::InMemoryExporter, peut être cette implémentation là)
        // C'est à mbserve de gérer les problèmatiques de persistences de data, configuration InMemoryExporter pour representer un buffer de données Modbus
        exportData(data);
    }

    return success;
}

void ModbusCollector::exportData(const TelemetryData& data) {
    std::lock_guard<std::mutex> lock(controlMutex_); // Reuse controlMutex for simplicity
    for (auto& exporter : exporters_) {
        if (exporter && exporter->is_connected()) {
            try {
                exporter->export_data(data);
            } catch (const std::exception& e) {
                LOG_ERROR("Exporter error for " + config_.id + ": " + e.what());
            }
        }
    }
}

void ModbusCollector::processControlMessages() {
    std::lock_guard<std::mutex> lock(controlMutex_);
    while (!controlQueue_.empty()) {
        auto msg = controlQueue_.front();
        controlQueue_.pop();

        switch (msg.command) {
            case CollectorCommand::PAUSE:
                paused_ = true;
                LOG_INFO("Collector paused: " + config_.id);
                break;
            case CollectorCommand::RESUME:
                paused_ = false;
                LOG_INFO("Collector resumed: " + config_.id);
                controlCondition_.notify_all(); // Wake up the waiting thread
                break;
            case CollectorCommand::STOP:
                stopRequested_ = true;
                break;
            case CollectorCommand::SET_FREQUENCY:
                acquisitionPeriod_ = std::chrono::milliseconds(msg.parameter);
                LOG_INFO("Frequency updated for " + config_.id + ": " + std::to_string(msg.parameter) + "ms");
                break;
        }
    }
}

} // namespace modbustt