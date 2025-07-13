# Changelog

## Version 0.0.1 - 2025-07-13

### Fonctionnalités Initiales

#### Architecture Multi-threadée
- **Thread d'Acquisition (ACKx)** : Collecte cyclique des données Modbus TCP par ligne de production
- **Thread de Publication (Publisher)** : Agrégation et publication des données via MQTT
- **Thread de Configuration (Config)** : Reconfiguration dynamique via commandes MQTT
- **Thread Principal (Main)** : Gestion du cycle de vie et surveillance de la configuration

#### Communication Modbus TCP
- Support des registres de maintien (holding registers)
- Support des registres d'entrée (input registers)
- Support des bobines (coils)
- Support des entrées discrètes (discrete inputs)
- Reconnexion automatique en cas de perte de connexion
- Configuration flexible des adresses, types et mise à l'échelle

#### Communication MQTT
- Publication périodique des données au format JSON
- Souscription aux commandes de reconfiguration
- Support de l'authentification (username/password)
- Gestion de la qualité de service (QoS)
- Reconnexion automatique

#### Reconfiguration Dynamique
- Pause/reprise des lignes de production
- Modification de la cadence d'acquisition
- Arrêt/redémarrage des lignes
- Rechargement automatique du fichier de configuration
- Commandes via MQTT en temps réel

#### Gestion de Configuration
- Fichier de configuration YAML
- Surveillance des modifications de fichier
- Validation des paramètres
- Configuration par ligne de production

#### Système de Journalisation
- Niveaux de log configurables (DEBUG, INFO, WARN, ERROR)
- Sortie console et fichier
- Horodatage précis avec millisecondes
- Journalisation thread-safe

### Technologies et Dépendances

- **C++17** : Standard moderne avec support multi-threading
- **libmodbus** : Bibliothèque robuste pour Modbus TCP
- **paho-mqtt-cpp** : Client MQTT officiel Eclipse
- **yaml-cpp** : Parser YAML moderne
- **nlohmann/json** : Bibliothèque JSON header-only
- **CMake** : Système de build moderne
- **CPack** : Création de packages

### Compilation et Installation

- Script de build automatique (`build.sh`)
- Support Ubuntu/Debian
- Installation automatique des dépendances
- Création de packages TGZ
- Documentation d'installation complète

### Documentation

- **README.md** : Guide d'utilisation principal
- **docs/INSTALL.md** : Guide d'installation détaillé
- **docs/API.md** : Documentation API MQTT
- **CHANGELOG.md** : Historique des versions

### Tests

- Tests unitaires de base
- Validation du multi-threading
- Tests de compilation

### Sécurité

- Authentification MQTT
- Validation des commandes
- Gestion sécurisée des fichiers de configuration
- Isolation des processus

### Performance

- Architecture multi-threadée optimisée
- Gestion efficace de la mémoire
- Reconnexions automatiques
- Limitation des queues de données

## Fonctionnalités Prévues pour les Versions Futures

### Version 1.1.0 (Prévue)
- Support MQTT over TLS/SSL
- Interface web de monitoring
- Métriques de performance
- Support de bases de données pour l'historique

### Version 1.2.0 (Prévue)
- Support Modbus RTU
- Plugins pour protocoles additionnels
- API REST pour la configuration
- Dashboard temps réel

### Version 2.0.0 (Prévue)
- Architecture microservices
- Support Kubernetes
- Intégration cloud native
- Machine learning pour la prédiction

## Notes de Compatibilité

### Systèmes Supportés
- Ubuntu 20.04 LTS et plus récent
- Debian 10 et plus récent
- CentOS 8 et plus récent (avec adaptations)

### Prérequis Minimaux
- CPU : 2 cœurs
- RAM : 1 GB
- Espace disque : 500 MB
- Réseau : Accès TCP aux équipements

### Dépendances Externes
- Broker MQTT (Mosquitto, HiveMQ, etc.)
- Équipements Modbus TCP compatibles
- Réseau TCP/IP stable

## Problèmes Connus

### Version 1.0.0
- L'installation de paho-mqtt-cpp peut nécessiter une compilation depuis les sources sur certaines distributions
- La reconfiguration dynamique complète (ajout/suppression de lignes) nécessite un redémarrage
- Les logs peuvent devenir volumineux en mode DEBUG

### Solutions de Contournement
- Utiliser le script `build.sh` pour l'installation automatique des dépendances
- Planifier les reconfigurations majeures pendant les arrêts de maintenance
- Configurer la rotation des logs système

## Support et Maintenance

### Cycle de Vie
- Support LTS : 3 ans pour les versions majeures
- Mises à jour de sécurité : Selon les besoins
- Nouvelles fonctionnalités : Versions mineures trimestrielles

### Contribution
- Code source disponible
- Issues et pull requests acceptées
- Documentation des API publiques
- Tests requis pour les nouvelles fonctionnalités

