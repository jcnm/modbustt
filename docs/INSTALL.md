# Guide d'Installation Détaillé

## Prérequis Système

### Système d'Exploitation
- Ubuntu 20.04 LTS ou plus récent
- Debian 10 ou plus récent
- CentOS 8 ou plus récent (avec adaptations des commandes d'installation)

### Ressources Minimales
- CPU : 2 cœurs minimum, 4 cœurs recommandés
- RAM : 1 GB minimum, 2 GB recommandés
- Espace disque : 500 MB pour l'installation, 1 GB recommandé pour les logs
- Réseau : Accès TCP aux équipements Modbus et au broker MQTT

## Installation des Dépendances

### Ubuntu/Debian

```bash
# Mise à jour du système
sudo apt-get update && sudo apt-get upgrade -y

# Outils de développement
sudo apt-get install -y build-essential cmake pkg-config git

# Bibliothèques système
sudo apt-get install -y libmodbus-dev libyaml-cpp-dev nlohmann-json3-dev

# Dépendances pour paho-mqtt-cpp
sudo apt-get install -y libssl-dev
```

### Installation de paho-mqtt-cpp

Si paho-mqtt-cpp n'est pas disponible via le gestionnaire de paquets :

```bash
# Créer un répertoire de travail temporaire
mkdir -p ~/temp-build && cd ~/temp-build

# Installer paho-mqtt-c
git clone https://github.com/eclipse/paho.mqtt.c.git
cd paho.mqtt.c
cmake -Bbuild -H. -DPAHO_ENABLE_TESTING=OFF -DPAHO_BUILD_STATIC=ON -DPAHO_WITH_SSL=ON
cmake --build build/
sudo cmake --build build/ --target install
cd ..

# Installer paho-mqtt-cpp
git clone https://github.com/eclipse/paho.mqtt.cpp.git
cd paho.mqtt.cpp
cmake -Bbuild -H. -DPAHO_BUILD_STATIC=ON -DPAHO_WITH_SSL=ON
cmake --build build/
sudo cmake --build build/ --target install

# Mettre à jour le cache des bibliothèques
sudo ldconfig

# Nettoyer
cd ~ && rm -rf ~/temp-build
```

## Compilation du Projet

### Méthode Automatique (Recommandée)

```bash
# Extraire l'archive du projet
tar -xzf supervision-system.tar.gz
cd supervision-system

# Lancer la compilation automatique
./build.sh
```

### Méthode Manuelle

```bash
# Extraire l'archive du projet
tar -xzf supervision-system.tar.gz
cd supervision-system

# Créer le répertoire de build
mkdir -p build && cd build

# Configuration CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Compilation
make -j$(nproc)

# Tests (optionnel)
make test
```

## Configuration

### Fichier de Configuration Principal

Copier et adapter le fichier de configuration :

```bash
# Copier la configuration par défaut
cp config/config.yaml config/config_production.yaml

# Éditer la configuration
nano config/config_production.yaml
```

### Configuration MQTT

Adapter les paramètres MQTT selon votre environnement :

```yaml
mqtt:
  broker: "192.168.1.50"      # IP de votre broker MQTT
  port: 1883                  # Port MQTT (1883 non-sécurisé, 8883 sécurisé)
  client_id: "supervision_001" # ID unique pour ce client
  username: "supervision"     # Nom d'utilisateur MQTT (optionnel)
  password: "motdepasse"      # Mot de passe MQTT (optionnel)
```

### Configuration des Équipements

Pour chaque équipement Modbus, ajouter une section :

```yaml
production_lines:
  - id: "LIGNE_01"
    ip: "192.168.1.100"       # IP de l'automate
    port: 502                 # Port Modbus TCP
    unit_id: 1                # ID d'unité Modbus
    acquisition_frequency_ms: 200  # Fréquence d'acquisition en ms
    enabled: true             # Activer cette ligne
    registers:
      - address: 40001        # Adresse Modbus (format standard)
        name: "temperature"   # Nom du point de donnée
        type: "holding"       # Type de registre
        scale: 0.1           # Facteur d'échelle
        offset: 0.0          # Offset
```

## Installation en Production

### Création d'un Utilisateur Système

```bash
# Créer un utilisateur dédié
sudo useradd -r -s /bin/false supervision
sudo mkdir -p /opt/supervision-system
sudo chown supervision:supervision /opt/supervision-system
```

### Installation des Fichiers

```bash
# Copier l'exécutable
sudo cp build/src/supervision_system /opt/supervision-system/
sudo chmod +x /opt/supervision-system/supervision_system

# Copier la configuration
sudo cp config/config_production.yaml /opt/supervision-system/config.yaml
sudo chown supervision:supervision /opt/supervision-system/config.yaml
sudo chmod 600 /opt/supervision-system/config.yaml

# Créer le répertoire de logs
sudo mkdir -p /var/log/supervision-system
sudo chown supervision:supervision /var/log/supervision-system
```

### Service Systemd

Créer un service systemd pour le démarrage automatique :

```bash
sudo tee /etc/systemd/system/supervision-system.service > /dev/null << 'EOF'
[Unit]
Description=Système de Supervision d'Atelier
After=network.target

[Service]
Type=simple
User=supervision
Group=supervision
WorkingDirectory=/opt/supervision-system
ExecStart=/opt/supervision-system/supervision_system /opt/supervision-system/config.yaml
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF

# Recharger systemd et activer le service
sudo systemctl daemon-reload
sudo systemctl enable supervision-system
```

### Démarrage du Service

```bash
# Démarrer le service
sudo systemctl start supervision-system

# Vérifier le statut
sudo systemctl status supervision-system

# Voir les logs
sudo journalctl -u supervision-system -f
```

## Vérification de l'Installation

### Test de Connectivité Modbus

```bash
# Installer modbus-utils pour les tests
sudo apt-get install -y libmodbus-dev

# Tester la connexion à un équipement
modpoll -m tcp -a 1 -r 40001 -c 1 192.168.1.100
```

### Test de Connectivité MQTT

```bash
# Installer mosquitto-clients pour les tests
sudo apt-get install -y mosquitto-clients

# Tester la publication
mosquitto_pub -h 192.168.1.50 -t "test/topic" -m "test message"

# Tester la souscription
mosquitto_sub -h 192.168.1.50 -t "supervision/data"
```

### Vérification des Logs

```bash
# Logs du service
sudo journalctl -u supervision-system --since "1 hour ago"

# Logs applicatifs
sudo tail -f /var/log/supervision-system/supervision.log
```

## Dépannage

### Problèmes de Compilation

1. **Erreur CMake** : Vérifier que toutes les dépendances sont installées
2. **Erreur de linking** : S'assurer que `ldconfig` a été exécuté après l'installation des bibliothèques
3. **Erreur C++17** : Vérifier la version de GCC (minimum 7.0)

### Problèmes de Connexion

1. **Modbus** : Vérifier l'IP, le port et l'ID d'unité
2. **MQTT** : Vérifier les paramètres de connexion et les credentials
3. **Firewall** : S'assurer que les ports sont ouverts

### Problèmes de Performance

1. **CPU élevé** : Réduire la fréquence d'acquisition
2. **Mémoire** : Vérifier les fuites avec `valgrind`
3. **Réseau** : Monitorer la bande passante utilisée

## Maintenance

### Mise à Jour

```bash
# Arrêter le service
sudo systemctl stop supervision-system

# Sauvegarder la configuration
sudo cp /opt/supervision-system/config.yaml /opt/supervision-system/config.yaml.bak

# Remplacer l'exécutable
sudo cp nouveau_supervision_system /opt/supervision-system/supervision_system

# Redémarrer le service
sudo systemctl start supervision-system
```

### Sauvegarde

```bash
# Sauvegarder la configuration
sudo tar -czf supervision-backup-$(date +%Y%m%d).tar.gz \
    /opt/supervision-system/config.yaml \
    /var/log/supervision-system/
```

### Monitoring

```bash
# Surveiller l'utilisation des ressources
top -p $(pgrep supervision_system)

# Surveiller les connexions réseau
sudo netstat -tulpn | grep supervision_system
```

