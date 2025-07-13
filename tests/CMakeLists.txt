# Tests unitaires (optionnels pour cette version)
# Pour une implémentation future avec Google Test ou Catch2

# find_package(GTest REQUIRED)
# 
# set(TEST_SOURCES
#     test_modbus_data.cpp
#     test_config_manager.cpp
# )
# 
# add_executable(supervision_tests ${TEST_SOURCES})
# target_link_libraries(supervision_tests 
#     ${MODBUS_LIBRARIES}
#     yaml-cpp
#     paho-mqttpp3
#     paho-mqtt3as
#     Threads::Threads
#     GTest::GTest
#     GTest::Main
# )
# 
# add_test(NAME SupervisionTests COMMAND supervision_tests)

# Pour l'instant, créer un test simple
add_executable(simple_test simple_test.cpp)
target_link_libraries(simple_test Threads::Threads)

