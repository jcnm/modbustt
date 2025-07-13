#pragma once

#include "iexporter.h"
#include <memory>

// Forward declaration to avoid including zmq.hpp in the header
namespace zmq {
    class context_t;
    class socket_t;
}

namespace modbustt {
namespace exporters {

class ZmqExporter : public IExporter {
public:
    void configure(const nlohmann::json& config) override;
    bool connect() override;
    void disconnect() override;
    void export_data(const TelemetryData& data) override;
    bool is_connected() const override { return connected_; }
private:
    std::unique_ptr<zmq::context_t> context_;
    std::unique_ptr<zmq::socket_t> socket_;
    bool connected_ = false;
};

} // namespace exporters
} // namespace modbustt