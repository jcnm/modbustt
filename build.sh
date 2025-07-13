#!/bin/bash

# Script de compilation du système de supervision sur macOS
# Ce script installe les dépendances via Homebrew et compile le projet avec CMake

set -e
WORK_DIR=$(pwd)
echo "=== Démarrage du script de compilation ==="
echo "Répertoire de travail: $WORK_DIR"
echo "=== Installation des dépendances via Homebrew ==="

# Vérification de Homebrew
if ! command -v brew &> /dev/null; then
    echo "Homebrew non trouvé. Installation..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
fi

brew update
# paho.mqtt.c dependencies
brew install openssl

# Installation des dépendances de base
brew install cmake git pkg-config

# Installation de libmodbus
brew install libmodbus

# Installation de yaml-cpp
brew install yaml-cpp

# Installation de nlohmann/json
brew install nlohmann-json
export CPLUS_INCLUDE_PATH="$HOME/.local/include:$CPLUS_INCLUDE_PATH"
export LIBRARY_PATH="$HOME/.local/lib:$LIBRARY_PATH"
export PKG_CONFIG_PATH="$HOME/.local/lib/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH="$HOME/.local/lib:$LD_LIBRARY_PATH"

# Installation de paho-mqtt-c et paho-mqtt-cpp
if [ ! pkg-config --exists paho-mqttpp3 ]; then
    echo "Installation de paho-mqtt-c et paho-mqtt-cpp via source..."

    TEMP_DIR=$(mktemp -d)
    cd "$TEMP_DIR"
    pwd
    echo "Répertoire temporaire créé: $TEMP_DIR"
    if [ ! -f "/usr/local/lib/libpaho-mqtt3as.a" ]; then
        # paho.mqtt.c
        git clone https://github.com/eclipse/paho.mqtt.c.git
        cd paho.mqtt.c
        sudo cmake -Bbuild -H. -DPAHO_BUILD_STATIC=ON \
        -DPAHO_ENABLE_TESTING=OFF -DPAHO_BUILD_DOCUMENTATION=OFF -DPAHO_WITH_SSL=TRUE
        sudo cmake --build build --target install
        cd ..
        echo "paho.mqtt.c installé"
    fi
    
    if [ ! -f "/usr/local/lib/libpaho-mqttpp3.a" ]; then
        # paho.mqtt.cpp
        echo "Installation de paho.mqtt.cpp..."
        echo "-----------------------------"
        git clone https://github.com/eclipse/paho.mqtt.cpp.git
        cd paho.mqtt.cpp
        sudo cmake -Bbuild -H. -DPAHO_BUILD_STATIC=ON\
        -DPAHO_BUILD_DOCUMENTATION=OFF -DPAHO_WITH_SSL=TRUE
        sudo cmake --build build --target install
        echo "paho.mqtt.cpp installé"
    fi
    cd /
    sudo rm -rf "$TEMP_DIR"
else
    echo "paho-mqtt-cpp déjà installé"
fi

echo "=== Compilation du projet ==="
pwd
cd "$WORK_DIR"
# Création du répertoire de build
echo "$(dirname "$0")"
cd "$WORK_DIR"
echo "Répertoire de travail: $(pwd)"
mkdir -p build
cd build

# Configuration avec CMake
cmake ..

# Compilation
make -j$(sysctl -n hw.ncpu)

echo "=== Compilation terminée ==="
echo "Exécutable créé: build/src/supervision_system"
echo "Pour lancer: cd build && ./src/supervision_system ../config/config.yaml"
cd ..
