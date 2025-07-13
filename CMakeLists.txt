cmake_minimum_required(VERSION 3.16)
project(SupervisionSystem VERSION 1.0.0 LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Répertoires d'installation locale (~/.local)
list(APPEND CMAKE_PREFIX_PATH "$ENV{HOME}/.local")
include_directories("/usr/local/include")
link_directories("/usr/local/lib")
list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/")  # ou /usr/local selon ta machine
link_directories("/opt/homebrew/lib/")           # ou /usr/local/lib
# Dépendances
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(OpenSSL REQUIRED)

# libmodbus via pkg-config
pkg_check_modules(MODBUS REQUIRED libmodbus)
include_directories(${MODBUS_INCLUDE_DIRS})

# En-têtes du projet
include_directories(include)

# Fichiers source du noyau supervision
set(SOURCES
    src/Logger.cpp
    src/ConfigManager.cpp    
    src/ConfigThread.cpp
    
)

# Importer la bibliothèque paho-mqtt C statique
add_library(paho-mqtt3as STATIC IMPORTED)
set_target_properties(paho-mqtt3as PROPERTIES
    IMPORTED_LOCATION "/usr/local/lib/libpaho-mqtt3as.a"
    INTERFACE_INCLUDE_DIRECTORIES "/usr/local/include"
)

# Importer la bibliothèque paho-mqtt C++ statique
add_library(paho-mqttpp3 STATIC IMPORTED)
set_target_properties(paho-mqttpp3 PROPERTIES
    IMPORTED_LOCATION "/usr/local/lib/libpaho-mqttpp3.a"
    INTERFACE_INCLUDE_DIRECTORIES "/usr/local/include"
)

# Bibliothèque "core" de l'application (logique non-modbus)
add_library(supervision_core ${SOURCES})

# Ajout de la sous-bibliothèque modbustt
add_subdirectory(lib/modbustt)

# Liens de la lib core
target_link_libraries(supervision_core
    yaml-cpp 
    nlohmann_json::nlohmann_json
    Threads::Threads
    paho-mqtt3as
    paho-mqttpp3
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Exécutable de test simple
add_executable(test_core tests/simple_test.cpp)
target_link_libraries(test_core supervision_core)

# Exécutable principal
add_executable(supervisor src/main.cpp)
target_link_libraries(supervisor 
    supervision_core
    modbustt # On lie notre nouvelle bibliothèque !
)

# Installation
install(TARGETS supervisor
    RUNTIME DESTINATION bin
)

install(FILES config/config.yaml
    DESTINATION etc/supervision-system
)

# Configuration du packaging
set(CPACK_PACKAGE_NAME "SupervisionSystem")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Système de supervision d'atelier de production automatisé")
set(CPACK_PACKAGE_VENDOR "Industrial Automation Solutions")
set(CPACK_GENERATOR "TGZ")

include(CPack)
