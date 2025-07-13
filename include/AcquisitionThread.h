#pragma once

#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <modbus/modbus.h>
#include "ModbusData.h"
#include "ConfigManager.h"

/**
 * Commandes de contrôle pour le thread d'acquisition
 */
enum class AcquisitionCommand {
    PAUSE,
    RESUME,
    STOP,
    SET_FREQUENCY
};

struct AcquisitionControlMessage {
    AcquisitionCommand command;
    int parameter = 0; // Pour SET_FREQUENCY, contient la nouvelle fréquence en ms
};

/**
 * Thread d'acquisition de données Modbus TCP
 */
class AcquisitionThread {
public:
    AcquisitionThread(const ProductionLineConfig& config);
    ~AcquisitionThread();
    
    // Gestion du cycle de vie du thread
    bool start();
    void stop();
    void join();
    void lock();
    void unlock();
    
    // Contrôle du thread
    void pause();
    void resume();
    void setFrequency(int frequencyMs);
    
    // Récupération des données
    bool hasData() const;
    ModbusData getData();
    
    // État du thread
    bool isRunning() const { return running_; }
    bool isPaused() const { return paused_; }
    const std::string& getLineId() const { return config_.id; }

private:
    void threadFunction();
    bool connectToModbus();
    void disconnectFromModbus();
    bool readRegisters();
    void processControlMessages();
    
    ProductionLineConfig config_;
    std::unique_ptr<std::thread> thread_;
    std::atomic<bool> running_;
    std::atomic<bool> paused_;
    std::atomic<bool> stopRequested_;
    
    // Communication Modbus
    modbus_t* modbusContext_;
    bool connected_;
    
    // File de données acquises
    std::queue<ModbusData> dataQueue_;
    mutable std::mutex dataMutex_;
    std::condition_variable dataCondition_;
    
    // File de commandes de contrôle
    std::queue<AcquisitionControlMessage> controlQueue_;
    std::mutex controlMutex_;
    std::condition_variable controlCondition_;
    
    // Timing
    std::chrono::milliseconds acquisitionPeriod_;
};

