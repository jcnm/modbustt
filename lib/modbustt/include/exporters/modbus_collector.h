#pragma once

#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <vector>
#include <modbus/modbus.h>
#include "config.h"
#include "telemetry_data.h"
#include "exporters/iexporter.h"

namespace modbustt {

enum class CollectorCommand { PAUSE, RESUME, STOP, SET_FREQUENCY };

struct CollectorControlMessage {
    CollectorCommand command;
    int parameter = 0;
};

class ModbusCollector {
public:
    ModbusCollector(const CollectorConfig& config);
    ~ModbusCollector();

    bool start();
    void stop();
    void join();

    void pause();
    void resume();
    void setFrequency(int frequencyMs);

    void addExporter(std::shared_ptr<exporters::IExporter> exporter);

    bool isRunning() const { return running_; }
    bool isPaused() const { return paused_; }
    const std::string& getId() const { return config_.id; }

private:
    void threadFunction();
    bool connectToModbus();
    void disconnectFromModbus();
    bool readRegisters();
    void processControlMessages();
    void exportData(const TelemetryData& data);

    CollectorConfig config_;
    std::unique_ptr<std::thread> thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};
    std::atomic<bool> stopRequested_{false};

    modbus_t* modbusContext_ = nullptr;
    bool connected_ = false;

    std::queue<CollectorControlMessage> controlQueue_;
    std::mutex controlMutex_;
    std::condition_variable controlCondition_;

    std::vector<std::shared_ptr<exporters::IExporter>> exporters_;
    std::chrono::milliseconds acquisitionPeriod_;
};

} // namespace modbustt