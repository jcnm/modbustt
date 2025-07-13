#include "AcquisitionThread.h"
#include "Logger.h"
#include <chrono>

AcquisitionThread::AcquisitionThread(const ProductionLineConfig& config)
    : config_(config)
    , running_(false)
    , paused_(false)
    , stopRequested_(false)
    , modbusContext_(nullptr)
    , connected_(false)
    , acquisitionPeriod_(config.acquisitionFrequencyMs) {
}

AcquisitionThread::~AcquisitionThread() {
    stop();
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
    disconnectFromModbus();
}

bool AcquisitionThread::start() {
    if (running_) {
        LOG_WARN("Thread d'acquisition déjà démarré pour la ligne: " + config_.id);
        return false;
    }
    
    if (!config_.enabled) {
        LOG_INFO("Ligne désactivée, thread non démarré: " + config_.id);
        return false;
    }
    
    running_ = true;
    stopRequested_ = false;
    paused_ = false;
    
    thread_ = std::make_unique<std::thread>(&AcquisitionThread::threadFunction, this);
    
    LOG_INFO("Thread d'acquisition démarré pour la ligne: " + config_.id);
    return true;
}

void AcquisitionThread::stop() {
    if (!running_) {
        return;
    }
    
    stopRequested_ = true;
    
    // Réveiller le thread s'il est en attente
    controlCondition_.notify_all();
    
    LOG_INFO("Arrêt demandé pour le thread d'acquisition: " + config_.id);
}

void AcquisitionThread::join() {
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
}

void AcquisitionThread::pause() {
    AcquisitionControlMessage msg;
    msg.command = AcquisitionCommand::PAUSE;
    
    {
        std::lock_guard<std::mutex> lock(controlMutex_);
        controlQueue_.push(msg);
    }
    controlCondition_.notify_one();
}

void AcquisitionThread::resume() {
    AcquisitionControlMessage msg;
    msg.command = AcquisitionCommand::RESUME;
    
    {
        std::lock_guard<std::mutex> lock(controlMutex_);
        controlQueue_.push(msg);
    }
    controlCondition_.notify_one();
}

void AcquisitionThread::setFrequency(int frequencyMs) {
    AcquisitionControlMessage msg;
    msg.command = AcquisitionCommand::SET_FREQUENCY;
    msg.parameter = frequencyMs;
    
    {
        std::lock_guard<std::mutex> lock(controlMutex_);
        controlQueue_.push(msg);
    }
    controlCondition_.notify_one();
}

bool AcquisitionThread::hasData() const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    return !dataQueue_.empty();
}

ModbusData AcquisitionThread::getData() {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    if (dataQueue_.empty()) {
        return ModbusData(); // Retourne une structure vide
    }
    
    ModbusData data = dataQueue_.front();
    dataQueue_.pop();
    return data;
}

void AcquisitionThread::threadFunction() {
    LOG_INFO("Thread d'acquisition en cours d'exécution pour: " + config_.id);
    
    while (!stopRequested_) {
        // Traiter les messages de contrôle
        processControlMessages();
        
        if (stopRequested_) break;
        
        // Si en pause, attendre
        if (paused_) {
            std::unique_lock<std::mutex> lock(controlMutex_);
            controlCondition_.wait(lock, [this] { return !paused_ || stopRequested_; });
            continue;
        }
        
        // Connexion Modbus si nécessaire
        if (!connected_) {
            if (!connectToModbus()) {
                // Attendre avant de réessayer
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
        }
        
        // Lecture des registres
        if (connected_ && readRegisters()) {
            // Les données ont été ajoutées à la queue dans readRegisters()
        }
        
        // Attendre la prochaine acquisition
        std::this_thread::sleep_for(acquisitionPeriod_);
    }
    
    disconnectFromModbus();
    running_ = false;
    
    LOG_INFO("Thread d'acquisition terminé pour: " + config_.id);
}

bool AcquisitionThread::connectToModbus() {
    if (modbusContext_) {
        disconnectFromModbus();
    }
    
    modbusContext_ = modbus_new_tcp(config_.ip.c_str(), config_.port);
    if (!modbusContext_) {
        LOG_ERROR("Impossible de créer le contexte Modbus pour " + config_.id + " (" + config_.ip + ":" + std::to_string(config_.port) + ")");
        return false;
    }
    
    // Configurer l'unité ID
    modbus_set_slave(modbusContext_, config_.unitId);
    
    // Configurer les timeouts
    modbus_set_response_timeout(modbusContext_, 1, 0); // 1 seconde
    modbus_set_byte_timeout(modbusContext_, 0, 500000); // 500ms
    
    if (modbus_connect(modbusContext_) == -1) {
        LOG_ERROR("Connexion Modbus échouée pour " + config_.id + ": " + modbus_strerror(errno));
        modbus_free(modbusContext_);
        modbusContext_ = nullptr;
        return false;
    }
    
    connected_ = true;
    LOG_INFO("Connexion Modbus établie pour " + config_.id);
    return true;
}

void AcquisitionThread::disconnectFromModbus() {
    if (modbusContext_) {
        modbus_close(modbusContext_);
        modbus_free(modbusContext_);
        modbusContext_ = nullptr;
    }
    connected_ = false;
}

bool AcquisitionThread::readRegisters() {
    if (!connected_ || !modbusContext_) {
        return false;
    }
    
    std::map<std::string, double> values;
    bool success = true;
    
    for (const auto& reg : config_.registers) {
        uint16_t rawValue[1];
        int result = -1;
        
        if (reg.type == "holding") {
            result = modbus_read_registers(modbusContext_, reg.address - 1, 1, rawValue);
        } else if (reg.type == "input") {
            result = modbus_read_input_registers(modbusContext_, reg.address - 1, 1, rawValue);
        } else if (reg.type == "coil") {
            uint8_t coilValue[1];
            result = modbus_read_bits(modbusContext_, reg.address - 1, 1, coilValue);
            if (result != -1) {
                rawValue[0] = coilValue[0];
            }
        } else if (reg.type == "discrete") {
            uint8_t discreteValue[1];
            result = modbus_read_input_bits(modbusContext_, reg.address - 1, 1, discreteValue);
            if (result != -1) {
                rawValue[0] = discreteValue[0];
            }
        }
        
        if (result == -1) {
            LOG_ERROR("Erreur lecture registre " + std::to_string(reg.address) + " pour " + config_.id + ": " + modbus_strerror(errno));
            success = false;
            
            // Marquer la connexion comme fermée en cas d'erreur
            connected_ = false;
            break;
        } else {
            // Appliquer la mise à l'échelle et l'offset
            double scaledValue = (static_cast<double>(rawValue[0]) * reg.scale) + reg.offset;
            values[reg.name] = scaledValue;
        }
    }
    
    if (success && !values.empty()) {
        // Créer et ajouter les données à la queue
        ModbusData data(config_.id, values);
        
        {
            std::lock_guard<std::mutex> lock(dataMutex_);
            dataQueue_.push(data);
            
            // Limiter la taille de la queue pour éviter l'accumulation
            while (dataQueue_.size() > 100) {
                dataQueue_.pop();
            }
        }
        dataCondition_.notify_one();
    }
    
    return success;
}

void AcquisitionThread::processControlMessages() {
    std::lock_guard<std::mutex> lock(controlMutex_);
    
    while (!controlQueue_.empty()) {
        AcquisitionControlMessage msg = controlQueue_.front();
        controlQueue_.pop();
        
        switch (msg.command) {
            case AcquisitionCommand::PAUSE:
                paused_ = true;
                LOG_INFO("Thread d'acquisition mis en pause: " + config_.id);
                break;
                
            case AcquisitionCommand::RESUME:
                paused_ = false;
                LOG_INFO("Thread d'acquisition repris: " + config_.id);
                break;
                
            case AcquisitionCommand::STOP:
                stopRequested_ = true;
                LOG_INFO("Arrêt demandé pour le thread d'acquisition: " + config_.id);
                break;
                
            case AcquisitionCommand::SET_FREQUENCY:
                acquisitionPeriod_ = std::chrono::milliseconds(msg.parameter);
                config_.acquisitionFrequencyMs = msg.parameter;
                LOG_INFO("Fréquence mise à jour pour " + config_.id + ": " + std::to_string(msg.parameter) + "ms");
                break;
        }
    }
}

