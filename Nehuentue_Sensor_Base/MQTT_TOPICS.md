# Arquitectura MQTT - Tópicos y Publicaciones

## 🏗️ Estructura de Tópicos

### Convención General
```
devices/{deviceId}/{categoria}/{subcategoria}
```

- **`deviceId`**: Identificador único del dispositivo (ej: `modbus-01`)
- **`categoria`**: Tipo de mensaje (`telemetry`, `status`, `event`, `cmd`)
- **`subcategoria`**: Especifica el dato o comando

## 📊 Tópicos Implementados

### 1. Telemetría (Datos del Sensor)

| Tópico | QoS | Retain | Frecuencia | Descripción |
|--------|-----|--------|------------|-------------|
| `devices/modbus-01/telemetry/temperature` | 0 | No | 10s | Temperatura medida |
| `devices/modbus-01/telemetry/current` | 0 | No | 10s | Corriente medida |

**Ejemplo de payload (temperatura)**:
```json
{
  "message": "Hello World",
  "uptime": 3600,
  "timestamp": 1234567890
}
```

### 2. Estado del Dispositivo

| Tópico | QoS | Retain | Frecuencia | Descripción |
|--------|-----|--------|------------|-------------|
| `devices/modbus-01/status` | 1 | Sí | 60s | Estado general del ESP32 |

**Payload de estado**:
```json
{
  "status": "online",
  "uptime": 3600,
  "heap": 245678,
  "rssi": -65,
  "ip": "192.168.1.123",
  "version": "1.0.0"
}
```

**Last Will (cuando se desconecta)**:
```json
{
  "status": "offline"
}
```

### 3. Eventos

| Tópico | QoS | Retain | Frecuencia | Descripción |
|--------|-----|--------|------------|-------------|
| `devices/modbus-01/event/error` | 1 | No | On-demand | Errores críticos |

**Payload de error**:
```json
{
  "type": "modbus_timeout",
  "severity": "warning",
  "message": "Timeout reading sensor",
  "timestamp": 1234567890
}
```

### 4. Comandos (Suscripciones)

| Tópico | QoS | Descripción |
|--------|-----|-------------|
| `devices/modbus-01/cmd/#` | 1 | Todos los comandos |
| `devices/modbus-01/cmd/reset` | 1 | Reiniciar ESP32 |
| `devices/modbus-01/cmd/recalibrate` | 1 | Recalibrar sensor |

**Payload de comando**:
```json
{
  "command": "reset",
  "params": {}
}
```

## 🔧 Configuración del Dispositivo

### ID del Dispositivo
Se configura en `wifiConfig.deviceId`:
```cpp
strcpy(wifiConfig.deviceId, "modbus-01");
```

### Construcción Automática de Tópicos
La función `buildMQTTTopics()` construye todos los tópicos dinámicamente:

```cpp
void buildMQTTTopics(const char* deviceId) {
    snprintf(mqttTopics.telemetryTemp, 64, 
             "devices/%s/telemetry/temperature", deviceId);
    
    snprintf(mqttTopics.telemetryCurrent, 64, 
             "devices/%s/telemetry/current", deviceId);
    
    snprintf(mqttTopics.status, 64, 
             "devices/%s/status", deviceId);
    
    snprintf(mqttTopics.eventError, 64, 
             "devices/%s/event/error", deviceId);
    
    snprintf(mqttTopics.cmdBase, 64, 
             "devices/%s/cmd/#", deviceId);
}
```

## 📝 Modo de Prueba: Hello World

### Publicación Actual (Testing)
```cpp
// Publica cada 10 segundos en telemetry/temperature
{
  "message": "Hello World",
  "uptime": 120,
  "timestamp": 1234567890
}
```

### Suscribirse desde Raspberry Pi

```bash
# Escuchar todos los mensajes del dispositivo
mosquitto_sub -h localhost -p 1883 -u mqttuser -P 1234 \
  -t "devices/modbus-01/#" -v

# Solo telemetría de temperatura
mosquitto_sub -h localhost -p 1883 -u mqttuser -P 1234 \
  -t "devices/modbus-01/telemetry/temperature" -v

# Solo estado
mosquitto_sub -h localhost -p 1883 -u mqttuser -P 1234 \
  -t "devices/modbus-01/status" -v

# Todos los dispositivos
mosquitto_sub -h localhost -p 1883 -u mqttuser -P 1234 \
  -t "devices/#" -v
```

### Enviar Comandos desde Raspberry Pi

```bash
# Reiniciar dispositivo
mosquitto_pub -h localhost -p 1883 -u mqttuser -P 1234 \
  -t "devices/modbus-01/cmd/reset" \
  -m '{"command":"reset","params":{}}'

# Recalibrar sensor
mosquitto_pub -h localhost -p 1883 -u mqttuser -P 1234 \
  -t "devices/modbus-01/cmd/recalibrate" \
  -m '{"command":"recalibrate","params":{"sensor":"temperature"}}'
```

## 🚀 Próximos Pasos

### 1. Activar WiFi/MQTT
Descomentar en `src/tasks.cpp`:
```cpp
#include <WiFi.h>
#include <PubSubClient.h>
```

Agregar a `platformio.ini`:
```ini
lib_deps = 
    knolleary/PubSubClient@^2.8
```

### 2. Publicar Datos Reales del Sensor

Modificar la sección "Hello World" por:

```cpp
if (millis() - lastPublish >= 10000) {
    lastPublish = millis();
    
    // Lee datos del sensor (protegido por mutex)
    if (dataMutex != NULL && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (sensorData.valid) {
            // Publica temperatura
            char tempPayload[128];
            snprintf(tempPayload, sizeof(tempPayload),
                     "{\"value\":%.2f,\"unit\":\"C\",\"timestamp\":%lu}",
                     sensorData.temperature, millis());
            
            mqttClient.publish(mqttTopics.telemetryTemp, tempPayload);
            
            // Publica corriente (si aplica)
            char currentPayload[128];
            snprintf(currentPayload, sizeof(currentPayload),
                     "{\"value\":%.2f,\"unit\":\"A\",\"timestamp\":%lu}",
                     sensorData.current, millis());
            
            mqttClient.publish(mqttTopics.telemetryCurrent, currentPayload);
        }
        xSemaphoreGive(dataMutex);
    }
}
```

### 3. Implementar Callback para Comandos

```cpp
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.printf("[MQTT] Mensaje recibido en: %s\n", topic);
    
    // Parse JSON
    StaticJsonDocument<256> doc;
    deserializeJson(doc, payload, length);
    
    const char* command = doc["command"];
    
    if (strcmp(command, "reset") == 0) {
        Serial.println("[MQTT] Comando: RESET");
        ESP.restart();
    }
    else if (strcmp(command, "recalibrate") == 0) {
        Serial.println("[MQTT] Comando: RECALIBRATE");
        // Lógica de recalibración
    }
}

// En setup de MQTT:
mqttClient.setCallback(mqttCallback);
```

## 📊 Dashboard Recomendado (Node-RED/Grafana)

### Node-RED Flow Example
```json
[
  {
    "type": "mqtt in",
    "topic": "devices/+/telemetry/temperature",
    "qos": "0",
    "broker": "mqtt-broker"
  },
  {
    "type": "json",
    "property": "payload"
  },
  {
    "type": "function",
    "func": "msg.payload = {temperature: msg.payload.value};\nreturn msg;"
  },
  {
    "type": "influxdb out",
    "database": "sensors"
  }
]
```

### Consulta Grafana
```sql
SELECT mean("value") 
FROM "temperature" 
WHERE ("device" = 'modbus-01') 
AND $timeFilter
GROUP BY time(1m)
```

## 🔐 Seguridad

### Best Practices

1. **Autenticación**: Siempre usa usuario/contraseña
2. **TLS/SSL**: En producción, usa MQTT sobre TLS (puerto 8883)
3. **ACL**: Configura permisos por tópico en Mosquitto
4. **Validación**: Valida todos los payloads JSON recibidos

### Configuración Mosquitto ACL

```conf
# /etc/mosquitto/acl.conf

# Usuario mqttuser puede publicar en devices/modbus-01/*
user mqttuser
topic write devices/modbus-01/#

# Usuario admin puede todo
user admin
topic #
```

## 📈 Métricas y Monitoreo

### Tópicos Adicionales Sugeridos

```
devices/modbus-01/metrics/cpu        # Uso de CPU
devices/modbus-01/metrics/memory     # Memoria libre
devices/modbus-01/metrics/uptime     # Tiempo de actividad
devices/modbus-01/debug/log          # Logs de debug
```

## 🎯 Resumen de Tópicos

| Categoría | Tópico Base | Uso |
|-----------|-------------|-----|
| **Telemetría** | `devices/modbus-01/telemetry/*` | Datos del sensor |
| **Estado** | `devices/modbus-01/status` | Estado del dispositivo |
| **Eventos** | `devices/modbus-01/event/*` | Errores y alertas |
| **Comandos** | `devices/modbus-01/cmd/*` | Control remoto |

**Ventajas de esta estructura**:
- ✅ Escalable: Fácil agregar más dispositivos (`modbus-02`, `modbus-03`...)
- ✅ Organizada: Categorías claras (telemetry, status, event, cmd)
- ✅ Flexible: Subcategorías específicas por tipo de dato
- ✅ Estándar: Compatible con Node-RED, Grafana, InfluxDB, etc.
