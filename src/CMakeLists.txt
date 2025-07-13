# Source files
set(SOURCES
    main.cpp
    AcquisitionThread.cpp
    PublisherThread.cpp
    ConfigThread.cpp
    ModbusData.cpp
    Logger.cpp
    ConfigManager.cpp
)

# Create executable
add_executable(supervision_system ${SOURCES})

# Link libraries
target_link_libraries(supervision_system
    ${MODBUS_LIBRARIES}
    yaml-cpp
    paho-mqttpp3
    paho-mqtt3as
    nlohmann_json::nlohmann_json
    Threads::Threads
)

# Compiler flags
target_compile_options(supervision_system PRIVATE ${MODBUS_CFLAGS_OTHER})

