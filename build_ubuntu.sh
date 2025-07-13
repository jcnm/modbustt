#!/bin/bash

# Script de compilation du système de supervision
# Ce script installe les dépendances et compile le projet

set -e

echo "=== Installation des dépendances ==="

# Mise à jour des paquets
sudo apt-get update

# Installation des dépendances de base
sudo apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    git

# Installation de libmodbus
sudo apt-get install -y libmodbus-dev

# Installation de yaml-cpp
sudo apt-get install -y libyaml-cpp-dev

# Installation de nlohmann/json
sudo apt-get install -y nlohmann-json3-dev

# Installation de paho-mqtt-cpp (peut nécessiter une compilation depuis les sources)
if ! pkg-config --exists paho-mqttpp3; then
    echo "Installation de paho-mqtt-cpp depuis les sources..."
    
    # Créer un répertoire temporaire
    TEMP_DIR=$(mktemp -d)
    cd "$TEMP_DIR"
    
    # Cloner et compiler paho-mqtt-c
    git clone https://github.com/eclipse/paho.mqtt.c.git
    cd paho.mqtt.c
    cmake -Bbuild -H. -DPAHO_ENABLE_TESTING=OFF -DPAHO_BUILD_STATIC=ON
    cmake --build build/ --target install
    sudo cmake --build build/ --target install
    cd ..
    
    # Cloner et compiler paho-mqtt-cpp
    git clone https://github.com/eclipse/paho.mqtt.cpp.git
    cd paho.mqtt.cpp
    cmake -Bbuild -H. -DPAHO_BUILD_STATIC=ON
    cmake --build build/ --target install
    sudo cmake --build build/ --target install
    
    # Nettoyer
    cd /
    rm -rf "$TEMP_DIR"
else
    echo "paho-mqtt-cpp déjà installé"
fi

echo "=== Compilation du projet ==="

# Retourner au répertoire du projet
cd "$(dirname "$0")"

# Créer le répertoire de build
mkdir -p build
cd build

# Configuration avec CMake
cmake ..

# Compilation
make -j$(nproc)

echo "=== Compilation terminée ==="
echo "Exécutable créé: build/src/supervision_system"
echo "Pour lancer: cd build && ./src/supervision_system ../config/config.yaml"
