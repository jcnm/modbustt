#pragma once

#include <string>
#include <vector>

namespace modbustt {

/**
 * @brief Configuration d'un registre Modbus à lire.
 */
struct RegisterConfig {
    int address;
    std::string name;
    std::string type; // "holding", "input", "coil", "discrete"
    double scale = 1.0;
    double offset = 0.0;
};

/**
 * @brief Configuration pour une connexion Modbus RTU.
 */
struct RtuConfig {
    std::string serial_port; // ex: "/dev/ttyUSB0"
    int baud_rate = 9600;
    char parity = 'N'; // 'N', 'E', 'O'
    int data_bits = 8;
    int stop_bits = 1;
};

/**
 * @brief Configuration complète pour un collecteur de données.
 */
struct CollectorConfig {
    std::string id;
    std::string protocol = "tcp"; // "tcp" ou "rtu"
    std::string ip_address;
    int port = 502;
    int unit_id = 1;
    RtuConfig rtu_settings;
    int acquisition_frequency_ms = 200;
    std::vector<RegisterConfig> registers;
};

} // namespace modbustt