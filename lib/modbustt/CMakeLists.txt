project(modbustt)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dépendances
find_package(PkgConfig REQUIRED)
find_package(nlohmann_json REQUIRED)

# libmodbus via pkg-config
pkg_check_modules(MODBUS REQUIRED libmodbus)

# libzmq via pkg-config (optionnel, pour ZmqExporter)
pkg_check_modules(ZMQ QUIET libzmq)

# Fichiers source de la bibliothèque
set(MODBUSTT_SOURCES
    src/modbus_collector.cpp
    src/exporters/file_exporter.cpp
    src/exporters/in_memory_exporter.cpp
    src/exporters/mqtt_exporter.cpp
    src/exporters/syslog_exporter.cpp
    src/exporters/tcp_exporter.cpp
)

if(ZMQ_FOUND)
    list(APPEND MODBUSTT_SOURCES src/exporters/zmq_exporter.cpp)
    message(STATUS "ZMQ found, building ZmqExporter.")
else()
    message(STATUS "ZMQ not found, skipping ZmqExporter.")
endif()

add_library(modbustt ${MODBUSTT_SOURCES})

target_include_directories(modbustt PUBLIC include ${MODBUS_INCLUDE_DIRS})

target_link_libraries(modbustt PUBLIC
    ${MODBUS_LIBRARIES}
    nlohmann_json::nlohmann_json
)

# Add paho-mqtt include dirs directly to the modbustt target
get_target_property(PAHO_INCLUDE_DIRS paho-mqttpp3 INTERFACE_INCLUDE_DIRECTORIES)
if(PAHO_INCLUDE_DIRS)
    target_include_directories(modbustt PUBLIC ${PAHO_INCLUDE_DIRS})
endif()

# Link paho-mqtt and its dependencies
target_link_libraries(modbustt PUBLIC
    paho-mqttpp3 # This should now work because include dirs are handled
    paho-mqtt3as
    OpenSSL::SSL OpenSSL::Crypto Threads::Threads # Their dependencies
)

if(ZMQ_FOUND)
    target_link_libraries(modbustt PUBLIC ${ZMQ_LIBRARIES})
    target_include_directories(modbustt PUBLIC ${ZMQ_INCLUDE_DIRS})
endif()