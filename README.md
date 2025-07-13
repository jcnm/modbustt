# Système de Supervision d'Atelier de Production Automatisé

## Description

Ce système de supervision permet de collecter des données en temps réel à partir de capteurs, moteurs et PLCs via Modbus TCP, de publier l'état combiné vers un système SCADA décentralisé ou le cloud industriel via MQTT, et de permettre une reconfiguration dynamique sans redémarrage.

## Architecture

Le système adopte une architecture modulaire et multi-threadée avec les composants suivants :

- **Threads d'Acquisition (ACKx)** : Un thread par ligne de production pour la collecte cyclique des données via Modbus TCP
- **Thread de Publication (Publisher)** : Agrégation et publication périodique des données via MQTT
- **Thread de Configuration (Config)** : Gestion de la reconfiguration dynamique via commandes MQTT
- **Thread Principal (Main)** : Gestion du cycle de vie et surveillance du fichier de configuration

## Technologies Utilisées

- **C++17** : Langage principal avec support multi-threading natif
- **libmodbus** : Communication Modbus TCP
- **paho-mqtt-cpp** : Client MQTT pour publication et souscription
- **yaml-cpp** : Parsing du fichier de configuration YAML
- **nlohmann/json** : Sérialisation/désérialisation JSON pour les messages MQTT

## Installation et Compilation

### Prérequis

- Ubuntu 20.04+ ou distribution Linux compatible
- GCC 9+ avec support C++17
- CMake 3.16+
- Accès sudo pour l'installation des dépendances

### Compilation Automatique

```bash
# Cloner ou extraire le projet
cd supervision-system

# Lancer le script de build (installe les dépendances et compile)
./build.sh
```

### Compilation Manuelle

```bash
# Installation des dépendances
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config
sudo apt-get install -y libmodbus-dev libyaml-cpp-dev nlohmann-json3-dev

# Pour paho-mqtt-cpp, voir le script build.sh pour l'installation depuis les sources

# Compilation
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

## Configuration

Le fichier `config/config.yaml` contient toute la configuration du système :

### Configuration MQTT

```yaml
mqtt:
  broker: "localhost"
  port: 1883
  client_id: "supervision_system_001"
  username: ""
  password: ""
  publish_topic: "supervision/data"
  command_topic: "supervision/commands"
  publish_frequency_ms: 800
  qos: 1
```

### Configuration des Lignes de Production

```yaml
production_lines:
  - id: "ACK1"
    ip: "192.168.1.100"
    port: 502
    unit_id: 1
    acquisition_frequency_ms: 200
    enabled: true
    registers:
      - address: 40001
        name: "temperature"
        type: "holding"
        scale: 0.1
        offset: 0.0
```

### Types de Registres Supportés

- `holding` : Registres de maintien (fonction 03)
- `input` : Registres d'entrée (fonction 04)
- `coil` : Bobines (fonction 01)
- `discrete` : Entrées discrètes (fonction 02)

## Utilisation

### Démarrage

```bash
cd build
./src/supervision_system ../config/config.yaml
```

### Arrêt

Le système peut être arrêté proprement avec `Ctrl+C` ou en envoyant un signal SIGTERM.

### Logs

Les logs sont affichés sur la console et sauvegardés dans le fichier `supervision.log`.

## Communication MQTT

### Messages de Données Publiés

Le système publie les données sur le topic configuré (par défaut `supervision/data`) au format JSON :

```json
{
  "timestamp": 1678886400000,
  "production_lines": [
    {
      "id": "ACK1",
      "timestamp": 1678886400000,
      "data": {
        "temperature": 25.5,
        "pressure": 10.2,
        "motor_speed": 1500
      }
    }
  ]
}
```

### Commandes de Reconfiguration

Le système écoute les commandes sur le topic configuré (par défaut `supervision/commands`) :

#### Mettre en pause une ou plusieurs lignes

```json
{
  "command": "pause_line",
  "line_ids": ["ACK1", "ACK3"]
}
```

#### Reprendre une ou plusieurs lignes

```json
{
  "command": "resume_line",
  "line_ids": ["ACK1", "ACK3"]
}
```

#### Changer la cadence d'acquisition

```json
{
  "command": "set_cadence",
  "line_id": "ACK2",
  "cadence_ms": 100
}
```

#### Arrêter une ou plusieurs lignes

```json
{
  "command": "stop_line",
  "line_ids": ["ACK1"]
}
```

## Reconfiguration Dynamique

Le système supporte plusieurs types de reconfiguration sans redémarrage :

1. **Modification du fichier config.yaml** : Le système détecte automatiquement les changements et recharge la configuration
2. **Commandes MQTT** : Contrôle en temps réel des lignes de production
3. **Gestion des erreurs** : Reconnexion automatique en cas de perte de connexion Modbus ou MQTT

## Gestion des Erreurs

- **Reconnexion automatique** pour les connexions Modbus et MQTT
- **Journalisation complète** de tous les événements et erreurs
- **Validation des données** de configuration et des messages MQTT
- **Gestion propre des threads** avec arrêt ordonné

## Sécurité

- Support de l'authentification MQTT (username/password)
- Validation des commandes MQTT reçues
- Gestion sécurisée des accès aux fichiers de configuration

## Tests

```bash
cd build
make test
```

## Création d'un Package

```bash
cd build
make package
```

Cela créera une archive `SupervisionSystem-1.0.0-Linux.tar.gz` contenant l'exécutable et les fichiers de configuration.

## Structure du Projet

```
supervision-system/
├── CMakeLists.txt          # Configuration CMake principale
├── build.sh                # Script de compilation automatique
├── README.md               # Cette documentation
├── config/
│   └── config.yaml         # Fichier de configuration
├── include/                # Headers C++
│   ├── AcquisitionThread.h
│   ├── PublisherThread.h
│   ├── ConfigThread.h
│   ├── ConfigManager.h
│   ├── ModbusData.h
│   └── Logger.h
├── src/                    # Sources C++
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── AcquisitionThread.cpp
│   ├── PublisherThread.cpp
│   ├── ConfigThread.cpp
│   ├── ConfigManager.cpp
│   ├── ModbusData.cpp
│   └── Logger.cpp
├── tests/                  # Tests unitaires
│   ├── CMakeLists.txt
│   └── simple_test.cpp
├── docs/                   # Documentation additionnelle
└── build/                  # Répertoire de compilation
```

## Support et Maintenance

Ce système a été conçu pour être robuste et maintenable :

- Code modulaire et bien documenté
- Architecture extensible pour de nouvelles fonctionnalités
- Gestion complète des erreurs et logging
- Configuration flexible via YAML
- Tests automatisés

## Licence

Ce projet est fourni sous licence open-source. Voir le fichier LICENSE pour plus de détails.

## Contribution

Les contributions sont les bienvenues. Veuillez suivre les bonnes pratiques de développement C++ et ajouter des tests pour toute nouvelle fonctionnalité.

