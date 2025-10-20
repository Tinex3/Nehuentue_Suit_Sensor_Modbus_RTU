# 📡 Configuración de Tópicos MQTT y Comandos

## 🎯 Estructura de Tópicos

### **Tópicos de Publicación (ESP32 → Broker)**

| Tópico | QoS | Retain | Frecuencia | Descripción |
|--------|-----|--------|------------|-------------|
| `devices/{deviceId}/telemetry` | 0 | No | Configurable (default: 60s) | Datos de sensores (temperatura, corriente, etc.) |
| `devices/{deviceId}/status` | 1 | Sí | Configurable (default: 60s) | Estado completo del sistema |
| `devices/{deviceId}/event/connect` | 1 | Sí | Al conectar | Evento de conexión inicial |
| `devices/{deviceId}/event/error` | 0 | No | Al ocurrir error | Eventos de error MQTT |

### **Tópicos de Suscripción (Broker → ESP32)**

| Tópico | Descripción |
|--------|-------------|
| `devices/{deviceId}/cmd/#` | Comandos (wildcard para todos los sub-tópicos) |

---

## 📊 Formatos de Payload

### 1. **Telemetría** (`devices/modbus-01/telemetry`)

```json
{
  "device_id": "modbus-01",
  "device_type": "modbus_sensor",
  "timestamp": 1729382400,
  "datetime": "2025-10-19T15:30:00-03:00",
  "sensor": {
    "type": "temperature",
    "value": 25.5,
    "unit": "celsius",
    "modbus_address": 1
  },
  "meta": {
    "uptime": 12345,
    "rssi": -65,
    "firmware": "1.0.0",
    "power_source": "USB"
  }
}
```

**Tipos de sensores soportados:**
- `"temperature"` - Temperatura (celsius)
- `"current"` - Corriente (ampere)
- `"voltage"` - Voltaje (volt) - futuro
- `"humidity"` - Humedad (percent) - futuro

---

### 2. **Estado** (`devices/modbus-01/status`)

```json
{
  "device_id": "modbus-01",
  "device_type": "modbus_sensor",
  "timestamp": 1729382400,
  "datetime": "2025-10-19T15:30:00-03:00",
  "status": "online",
  "network": {
    "ip": "192.168.1.100",
    "rssi": -65,
    "ssid": "Amanda 2.4G",
    "mac": "A1:B2:C3:D4:E5:F6"
  },
  "system": {
    "uptime": 12345,
    "heap_free": 280000,
    "heap_total": 327680,
    "cpu_freq": 160,
    "firmware": "1.0.0",
    "power_source": "USB",
    "rtc_synced": true
  },
  "modbus": {
    "status": "active",
    "sensors_connected": 1,
    "last_read": 1729382395
  }
}
```

---

### 3. **Evento de Conexión** (`devices/modbus-01/event/connect`)

```json
{
  "device_id": "modbus-01",
  "device_type": "modbus_sensor",
  "timestamp": 1729382400,
  "datetime": "2025-10-19T15:30:00-03:00",
  "event": "device_connected",
  "network": {
    "ip": "192.168.1.100",
    "rssi": -65,
    "ssid": "Amanda 2.4G",
    "mac": "A1:B2:C3:D4:E5:F6"
  },
  "system": {
    "firmware": "1.0.0",
    "hardware": "ESP32-C3",
    "chip_model": "ESP32-C3",
    "chip_revision": 3,
    "cpu_freq": 160,
    "flash_size": 4194304,
    "power_source": "USB"
  },
  "boot": {
    "reason": "power_on",
    "rtc_synced": true
  }
}
```

**Boot reasons:**
- `"power_on"` - Encendido inicial
- `"software"` - Reset por software
- `"watchdog"` - Reset por watchdog
- `"panic"` - Reset por panic
- `"brownout"` - Reset por caída de voltaje
- `"deep_sleep"` - Despertar de deep sleep

---

### 4. **Evento de Error** (`devices/modbus-01/event/error`)

```json
{
  "type": "mqtt_connection_failed",
  "severity": "error",
  "message": "ESP32 fue rechazado por credenciales inválidas",
  "state": 4,
  "state_name": "BAD_CREDENTIALS",
  "timestamp": 1729382400,
  "datetime": "2025-10-19T15:30:00-03:00"
}
```

---

## 🎮 Comandos MQTT

### **1. Cambiar Intervalos de Publicación**

**Tópico:** `devices/modbus-01/cmd/config`

**Payload:**
```json
{
  "telemetry_interval": 30000,
  "status_interval": 120000
}
```

**Parámetros:**
- `telemetry_interval`: Intervalo de telemetría en milisegundos (1000-3600000 ms = 1s-1h)
- `status_interval`: Intervalo de estado en milisegundos (1000-3600000 ms = 1s-1h)

**Ejemplo (cambiar a 30 segundos):**
```bash
mosquitto_pub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/cmd/config" \
  -m '{"telemetry_interval":30000}'
```

**Ejemplo (cambiar ambos intervalos):**
```bash
mosquitto_pub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/cmd/config" \
  -m '{"telemetry_interval":10000,"status_interval":60000}'
```

---

### **2. Reiniciar Dispositivo**

**Tópico:** `devices/modbus-01/cmd/reboot`

**Payload:**
```json
{
  "action": "reboot"
}
```

**Ejemplo:**
```bash
mosquitto_pub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/cmd/reboot" \
  -m '{"action":"reboot"}'
```

---

### **3. Solicitar Estado Inmediato**

**Tópico:** `devices/modbus-01/cmd/status`

**Payload:**
```json
{
  "action": "get_status"
}
```

**Ejemplo:**
```bash
mosquitto_pub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/cmd/status" \
  -m '{"action":"get_status"}'
```

---

## 🔧 Configuración por Defecto

```cpp
mqttConfig.telemetryInterval = 60000;  // 60 segundos (1 minuto)
mqttConfig.statusInterval = 60000;     // 60 segundos (1 minuto)
```

---

## 📝 Ejemplos de Uso con Mosquitto

### **Suscribirse a todos los mensajes de un dispositivo:**
```bash
mosquitto_sub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/#" -v
```

### **Suscribirse solo a telemetría:**
```bash
mosquitto_sub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/telemetry" -v
```

### **Suscribirse solo a eventos:**
```bash
mosquitto_sub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/event/#" -v
```

### **Cambiar intervalo a 2 minutos (120 segundos):**
```bash
mosquitto_pub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/cmd/config" \
  -m '{"telemetry_interval":120000,"status_interval":120000}'
```

### **Cambiar intervalo a 10 segundos (solo telemetría):**
```bash
mosquitto_pub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/cmd/config" \
  -m '{"telemetry_interval":10000}'
```

---

## 🌐 Integración con Node-RED

### **Flow de ejemplo para cambiar intervalos:**

```json
[
  {
    "id": "config_node",
    "type": "mqtt out",
    "topic": "devices/modbus-01/cmd/config",
    "qos": "0",
    "broker": "mqtt_broker"
  },
  {
    "id": "inject_30s",
    "type": "inject",
    "payload": "{\"telemetry_interval\":30000}",
    "payloadType": "str",
    "topic": "",
    "wires": [["config_node"]]
  }
]
```

---

## ⚙️ Variables Globales

```cpp
// En tasks.h
struct MQTTConfig {
    unsigned long telemetryInterval;  // Intervalo de publicación de telemetría (ms)
    unsigned long statusInterval;     // Intervalo de publicación de estado (ms)
};

extern MQTTConfig mqttConfig;
```

---

## 🔐 Seguridad

- **Usuario MQTT:** `mqttuser`
- **Contraseña:** `1234` (cambiar en producción)
- **Puerto:** `1883` (sin TLS) - Considerar usar `8883` con TLS en producción
- **Last Will:** Topic `devices/{deviceId}/status` con payload `{"status":"offline"}`

---

## 📌 Notas Importantes

1. ✅ **Intervalo mínimo:** 1 segundo (1000 ms)
2. ✅ **Intervalo máximo:** 1 hora (3600000 ms)
3. ✅ **Persistencia:** Los intervalos se resetean al reiniciar el ESP32
4. ⏳ **TODO:** Guardar configuración de intervalos en EEPROM para persistencia
5. 🕒 **Timestamps:** Usa NTP sincronizado con zona horaria Chile (UTC-3)
6. 🔄 **Re-sincronización NTP:** Cada 24 horas automáticamente

---

## 🚀 Roadmap Futuro

- [ ] Persistencia de intervalos en EEPROM
- [ ] Comando para calibrar sensores
- [ ] Comando para cambiar WiFi/MQTT sin recompilar
- [ ] Soporte para múltiples sensores Modbus
- [ ] Modo de bajo consumo (deep sleep)
- [ ] Actualización OTA (Over-The-Air)
- [ ] TLS/SSL para MQTT
