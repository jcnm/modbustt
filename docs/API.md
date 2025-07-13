# Documentation API MQTT

## Vue d'Ensemble

Le système de supervision communique via MQTT pour la publication des données et la réception des commandes de contrôle. Cette documentation décrit les formats de messages et les topics utilisés.

## Configuration MQTT

### Paramètres de Connexion

```yaml
mqtt:
  broker: "mqtt.example.com"
  port: 1883
  client_id: "supervision_system_001"
  username: "supervision_user"
  password: "supervision_password"
  publish_topic: "supervision/data"
  command_topic: "supervision/commands"
  publish_frequency_ms: 800
  qos: 1
```

## Messages de Données (Publication)

### Topic de Publication

Par défaut : `supervision/data`

### Format des Messages

Le système publie les données au format JSON avec la structure suivante :

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
        "motor_speed": 1500,
        "pump_status": 1
      }
    },
    {
      "id": "ACK2",
      "timestamp": 1678886400100,
      "data": {
        "temperature": 26.1,
        "pressure": 10.5,
        "flow_rate": 15.2,
        "valve_status": 0
      }
    }
  ]
}
```

### Description des Champs

- `timestamp` : Horodatage global du message (millisecondes depuis epoch Unix)
- `production_lines` : Tableau des lignes de production
  - `id` : Identifiant unique de la ligne de production
  - `timestamp` : Horodatage spécifique à cette ligne
  - `data` : Objet contenant les valeurs des points de données

### Fréquence de Publication

La fréquence de publication est configurable via le paramètre `publish_frequency_ms` (par défaut 800ms).

## Messages de Commandes (Souscription)

### Topic de Commandes

Par défaut : `supervision/commands`

### Format Général

Toutes les commandes suivent le format JSON suivant :

```json
{
  "command": "nom_de_la_commande",
  "paramètres": "valeurs_spécifiques"
}
```

## Commandes Disponibles

### 1. Mettre en Pause des Lignes

Suspend l'acquisition de données pour une ou plusieurs lignes.

```json
{
  "command": "pause_line",
  "line_ids": ["ACK1", "ACK3"]
}
```

**Paramètres :**
- `line_ids` : Tableau des identifiants de lignes à mettre en pause

**Effet :** Les threads d'acquisition des lignes spécifiées sont suspendus.

### 2. Reprendre des Lignes

Reprend l'acquisition de données pour des lignes en pause.

```json
{
  "command": "resume_line",
  "line_ids": ["ACK1", "ACK3"]
}
```

**Paramètres :**
- `line_ids` : Tableau des identifiants de lignes à reprendre

**Effet :** Les threads d'acquisition des lignes spécifiées reprennent leur fonctionnement.

### 3. Modifier la Cadence d'Acquisition

Change la fréquence d'acquisition pour une ligne spécifique.

```json
{
  "command": "set_cadence",
  "line_id": "ACK2",
  "cadence_ms": 100
}
```

**Paramètres :**
- `line_id` : Identifiant de la ligne à modifier
- `cadence_ms` : Nouvelle fréquence d'acquisition en millisecondes

**Effet :** La fréquence d'acquisition de la ligne est modifiée immédiatement.

### 4. Arrêter des Lignes

Arrête complètement l'acquisition pour une ou plusieurs lignes.

```json
{
  "command": "stop_line",
  "line_ids": ["ACK1"]
}
```

**Paramètres :**
- `line_ids` : Tableau des identifiants de lignes à arrêter

**Effet :** Les threads d'acquisition des lignes spécifiées sont arrêtés et supprimés.

### 5. Redémarrer des Lignes

Redémarre l'acquisition pour des lignes précédemment arrêtées.

```json
{
  "command": "restart_line",
  "line_ids": ["ACK1"],
  "config": {
    "ip": "192.168.1.100",
    "port": 502,
    "unit_id": 1,
    "acquisition_frequency_ms": 200,
    "registers": [
      {
        "address": 40001,
        "name": "temperature",
        "type": "holding",
        "scale": 0.1,
        "offset": 0.0
      }
    ]
  }
}
```

**Paramètres :**
- `line_ids` : Tableau des identifiants de lignes à redémarrer
- `config` : Configuration complète de la ligne (optionnel, utilise la config par défaut si absent)

## Messages de Statut (Publication)

### Topic de Statut

`supervision/status`

### Format des Messages de Statut

```json
{
  "timestamp": 1678886400000,
  "system_status": "running",
  "lines_status": [
    {
      "id": "ACK1",
      "status": "running",
      "last_acquisition": 1678886399800,
      "error_count": 0
    },
    {
      "id": "ACK2",
      "status": "paused",
      "last_acquisition": 1678886395000,
      "error_count": 2
    }
  ],
  "mqtt_status": {
    "connected": true,
    "last_publish": 1678886399900
  }
}
```

### États Possibles

**Système :**
- `starting` : Démarrage en cours
- `running` : Fonctionnement normal
- `stopping` : Arrêt en cours
- `error` : Erreur système

**Lignes :**
- `running` : Acquisition en cours
- `paused` : En pause
- `stopped` : Arrêtée
- `error` : Erreur de connexion

## Gestion des Erreurs

### Messages d'Erreur

En cas d'erreur lors du traitement d'une commande, le système peut publier un message d'erreur :

```json
{
  "timestamp": 1678886400000,
  "error": {
    "command": "pause_line",
    "message": "Ligne non trouvée: ACK99",
    "code": "LINE_NOT_FOUND"
  }
}
```

### Codes d'Erreur

- `LINE_NOT_FOUND` : Ligne de production non trouvée
- `INVALID_COMMAND` : Commande non reconnue
- `INVALID_PARAMETERS` : Paramètres de commande invalides
- `MODBUS_ERROR` : Erreur de communication Modbus
- `CONFIG_ERROR` : Erreur de configuration

## Exemples d'Utilisation

### Client Python pour Envoyer des Commandes

```python
import paho.mqtt.client as mqtt
import json

def send_command(broker, port, topic, command):
    client = mqtt.Client()
    client.connect(broker, port, 60)
    
    message = json.dumps(command)
    client.publish(topic, message)
    client.disconnect()

# Exemple : Mettre en pause la ligne ACK1
command = {
    "command": "pause_line",
    "line_ids": ["ACK1"]
}

send_command("localhost", 1883, "supervision/commands", command)
```

### Client Python pour Recevoir des Données

```python
import paho.mqtt.client as mqtt
import json

def on_connect(client, userdata, flags, rc):
    print(f"Connecté avec le code {rc}")
    client.subscribe("supervision/data")

def on_message(client, userdata, msg):
    data = json.loads(msg.payload.decode())
    print(f"Données reçues: {data}")
    
    for line in data["production_lines"]:
        print(f"Ligne {line['id']}: {line['data']}")

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect("localhost", 1883, 60)
client.loop_forever()
```

### Intégration avec Node-RED

Exemple de flow Node-RED pour traiter les données :

```json
[
  {
    "id": "mqtt-in",
    "type": "mqtt in",
    "topic": "supervision/data",
    "broker": "mqtt-broker"
  },
  {
    "id": "json-parse",
    "type": "json"
  },
  {
    "id": "function-process",
    "type": "function",
    "func": "for (let line of msg.payload.production_lines) {\n    if (line.data.temperature > 30) {\n        node.warn(`Température élevée sur ${line.id}: ${line.data.temperature}°C`);\n    }\n}\nreturn msg;"
  }
]
```

## Sécurité

### Authentification

Le système supporte l'authentification MQTT via username/password :

```yaml
mqtt:
  username: "supervision_user"
  password: "secure_password"
```

### Chiffrement

Pour une communication sécurisée, utilisez MQTT over TLS :

```yaml
mqtt:
  broker: "secure-mqtt.example.com"
  port: 8883  # Port TLS
  use_tls: true
  ca_cert: "/path/to/ca.crt"
```

### Validation des Commandes

Le système valide toutes les commandes reçues :
- Format JSON valide
- Champs obligatoires présents
- Valeurs dans les plages acceptables
- Identifiants de lignes existants

## Monitoring et Debugging

### Topics de Debug

Pour le debugging, vous pouvez vous abonner aux topics suivants :

- `supervision/debug` : Messages de debug détaillés
- `supervision/errors` : Messages d'erreur uniquement
- `supervision/heartbeat` : Signal de vie du système

### Outils de Test

```bash
# Écouter toutes les données
mosquitto_sub -h localhost -t "supervision/+"

# Envoyer une commande de test
mosquitto_pub -h localhost -t "supervision/commands" -m '{"command":"pause_line","line_ids":["ACK1"]}'

# Surveiller les erreurs
mosquitto_sub -h localhost -t "supervision/errors"
```

