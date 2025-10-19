# ✅ ARQUITECTURA MQTT IMPLEMENTADA

## 🎯 Resumen de Implementación

Se ha implementado una **tarea dedicada a MQTT** con arquitectura de tópicos escalable y organizada.

## 📦 Estructura del Proyecto

```
Nehuentue_Sensor_Base/
├── include/
│   ├── tasks.h              ← WiFiConfig + MQTTTopics + mqttTask()
│   ├── modbus_rtu.h
│   └── eeprom_manager.h
├── src/
│   ├── tasks.cpp            ← mqttTask() + buildMQTTTopics()
│   ├── modbus_rtu.cpp
│   ├── eeprom_manager.cpp
│   └── main.cpp
└── docs/
    ├── MQTT_TOPICS.md       ← **NUEVA** Arquitectura de tópicos
    ├── MQTT_CONFIG.md
    ├── EEPROM_USAGE.md
    └── CHANGELOG.md
```

## 🏗️ Arquitectura de Tópicos

### Convención
```
devices/{deviceId}/{categoria}/{subcategoria}
```

### Tópicos Implementados

| Tópico | Frecuencia | QoS | Retain | Descripción |
|--------|------------|-----|--------|-------------|
| `devices/modbus-01/telemetry/temperature` | 10s | 0 | No | **Hello World** (testing) |
| `devices/modbus-01/telemetry/current` | 10s | 0 | No | Corriente (futuro) |
| `devices/modbus-01/status` | 60s | 1 | Sí | Estado del ESP32 |
| `devices/modbus-01/event/error` | On-demand | 1 | No | Eventos de error |
| `devices/modbus-01/cmd/#` | - | 1 | - | Comandos (suscripción) |

## 🚀 Estado Actual: MODO TESTING

### Publicación Hello World

La tarea MQTT publica cada 10 segundos:

```json
{
  "message": "Hello World",
  "uptime": 120,
  "timestamp": 1234567890
}
```

**Tópico**: `devices/modbus-01/telemetry/temperature`

### Cómo Verificar desde Raspberry Pi

```bash
# Suscribirse a todos los mensajes del dispositivo
mosquitto_sub -h localhost -p 1883 -u mqttuser -P 1234 \
  -t "devices/modbus-01/#" -v

# Solo telemetría de temperatura (Hello World)
mosquitto_sub -h localhost -p 1883 -u mqttuser -P 1234 \
  -t "devices/modbus-01/telemetry/temperature"
```

**Salida esperada**:
```
devices/modbus-01/telemetry/temperature {"message":"Hello World","uptime":10,"timestamp":1234567890}
devices/modbus-01/telemetry/temperature {"message":"Hello World","uptime":20,"timestamp":1234567900}
devices/modbus-01/telemetry/temperature {"message":"Hello World","uptime":30,"timestamp":1234567910}
...
```

## 📝 Configuración

### WiFiConfig (EEPROM)

```cpp
struct WiFiConfig {
    char ssid[32];              // "MiWiFi"
    char password[64];          // "password123"
    char deviceId[32];          // "modbus-01" ← Nuevo
    char mqttServer[64];        // "192.168.1.25"
    uint16_t mqttPort;          // 1883
    char mqttUser[32];          // "mqttuser"
    char mqttPassword[64];      // "1234"
};
```

### MQTTTopics (Construidos Dinámicamente)

```cpp
struct MQTTTopics {
    char telemetryTemp[64];      // devices/modbus-01/telemetry/temperature
    char telemetryCurrent[64];   // devices/modbus-01/telemetry/current
    char status[64];             // devices/modbus-01/status
    char eventError[64];         // devices/modbus-01/event/error
    char cmdBase[64];            // devices/modbus-01/cmd/#
};
```

**Construcción automática**:
```cpp
buildMQTTTopics(wifiConfig.deviceId);
```

## 🔧 Activar WiFi/MQTT

### 1. Descomentar Librerías

En `src/tasks.cpp`, líneas ~125-128:
```cpp
#include <WiFi.h>
#include <PubSubClient.h>
WiFiClient espClient;
PubSubClient mqttClient(espClient);
```

### 2. Descomentar Código WiFi

Buscar y descomentar (líneas ~160-190):
```cpp
if (WiFi.status() != WL_CONNECTED) {
    // ... código WiFi
}
```

### 3. Descomentar Código MQTT

Buscar y descomentar (líneas ~200-250):
```cpp
if (!mqttClient.connected()) {
    // ... código MQTT
}

mqttClient.loop();
```

### 4. Descomentar Publicación

Líneas ~256-260:
```cpp
if (mqttClient.publish(mqttTopics.telemetryTemp, payload)) {
    Serial.println("[MQTT TASK] ✓ Publicado correctamente");
}
```

### 5. Agregar Librería en platformio.ini

```ini
[env:esp32-c3-devkitm-1]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
lib_deps = 
    knolleary/PubSubClient@^2.8
```

## 📊 Salida Esperada en Serial Monitor

```
==============================================
  Nehuentue Sensor Base - Modbus Master
  Arquitectura: 4 Tareas FreeRTOS
==============================================

[EEPROM TASK] Iniciada
[EEPROM TASK] Cargando configuración WiFi...
[EEPROM TASK] ✓ Configuración WiFi cargada con CRC válido
  SSID: MiWiFi
  Device ID: modbus-01
  MQTT Server: 192.168.1.25:1883
  MQTT User: mqttuser

[MQTT] Tópicos construidos:
  Telemetría Temp: devices/modbus-01/telemetry/temperature
  Telemetría Current: devices/modbus-01/telemetry/current
  Estado: devices/modbus-01/status
  Eventos: devices/modbus-01/event/error
  Comandos: devices/modbus-01/cmd/#

[MQTT TASK] Iniciada
[MQTT TASK] Conectando a WiFi...
  SSID: MiWiFi
[MQTT TASK] ✓ WiFi conectado
  IP: 192.168.1.123
  RSSI: -65 dBm
[MQTT TASK] ✓ MQTT conectado
  Broker: 192.168.1.25:1883
  Usuario: mqttuser
  Client ID: modbus-01-a1b2c3d4e5f6
  Suscrito a: devices/modbus-01/cmd/#

[MQTT TASK] Publicando Hello World...
  Tópico: devices/modbus-01/telemetry/temperature
  Payload: {"message":"Hello World","uptime":10,"timestamp":1234567890}
[MQTT TASK] ✓ Publicado correctamente

[MQTT TASK] Publicando estado...
  Payload: {"status":"online","uptime":60,"heap":245678,"rssi":-65}
[MQTT TASK] ✓ Estado publicado
```

## 🎯 Próximos Pasos

### Fase 1: Validar Hello World ✅
- [x] Crear tarea MQTT dedicada
- [x] Implementar arquitectura de tópicos
- [x] Publicar "Hello World" cada 10s
- [ ] **ACTIVAR WiFi/MQTT y verificar en Mosquitto**

### Fase 2: Datos Reales del Sensor
```cpp
// Reemplazar "Hello World" por:
if (sensorData.valid) {
    char payload[128];
    snprintf(payload, sizeof(payload),
             "{\"value\":%.2f,\"unit\":\"C\",\"timestamp\":%lu}",
             sensorData.temperature, millis());
    
    mqttClient.publish(mqttTopics.telemetryTemp, payload);
}
```

### Fase 3: Comandos MQTT
```cpp
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    if (strstr(topic, "/cmd/reset")) {
        ESP.restart();
    }
}
```

### Fase 4: Eventos de Error
```cpp
if (modbus_error) {
    char errorPayload[128];
    snprintf(errorPayload, sizeof(errorPayload),
             "{\"type\":\"modbus_timeout\",\"severity\":\"warning\"}");
    
    mqttClient.publish(mqttTopics.eventError, errorPayload);
}
```

## 🔐 Seguridad

- ✅ Autenticación con usuario/contraseña
- ✅ Last Will configurado (detecta desconexiones)
- ✅ Client ID único (MAC address)
- ⚠️ TLS/SSL pendiente (producción)

## 📈 Escalabilidad

Para agregar más dispositivos:

```cpp
// Dispositivo 2
strcpy(wifiConfig.deviceId, "modbus-02");

// Tópicos generados automáticamente:
// devices/modbus-02/telemetry/temperature
// devices/modbus-02/status
// etc.
```

## ✨ Ventajas de Esta Arquitectura

1. **Escalable**: Fácil agregar dispositivos (`modbus-01`, `modbus-02`...)
2. **Organizada**: Categorías claras (telemetry, status, event, cmd)
3. **Estándar**: Compatible con Node-RED, Grafana, InfluxDB
4. **Flexible**: Tópicos construidos dinámicamente
5. **Mantenible**: Tarea dedicada, código limpio
6. **Testing-Friendly**: "Hello World" para validación rápida

## 🐛 Troubleshooting

### Error: "MQTT desconectado"
- Verificar IP del broker (192.168.1.25)
- Verificar usuario/contraseña (mqttuser / 1234)
- Verificar que Mosquitto esté corriendo: `sudo systemctl status mosquitto`

### No veo mensajes en mosquitto_sub
- Verificar tópico correcto: `devices/modbus-01/telemetry/temperature`
- Verificar credenciales: `-u mqttuser -P 1234`
- Verificar que ESP32 esté conectado (ver Serial Monitor)

### WiFi no conecta
- Verificar SSID y contraseña en `wifiConfig`
- Verificar que red esté disponible
- Aumentar timeout de conexión

## 📚 Documentación Completa

Ver `MQTT_TOPICS.md` para:
- Detalles de cada tópico
- Ejemplos de payloads
- Comandos Mosquitto
- Integración con Node-RED/Grafana
- Configuración de seguridad
