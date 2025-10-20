# 📡 Comandos MQTT - Nehuentue Sensor v2.1

## 🔧 Configuración

**Firmware v2.1** - Sistema sin servidor web, configuración 100% por MQTT

### Credenciales Preconfiguradas

```
WiFi SSID:     Amanda 2.4G
WiFi Password: Gomezriquelmegomez12

MQTT Broker:   192.168.1.100:1883
MQTT User:     mqttuser
MQTT Password: 1234
Client ID:     nehuentue_sensor_001
```

### Tópicos MQTT

| Tópico | Descripción |
|--------|-------------|
| `nehuentue/{clientId}/cmd` | Enviar comandos al dispositivo |
| `nehuentue/{clientId}/response` | Recibir respuestas del dispositivo |
| `nehuentue/{clientId}/telemetry` | Datos de telemetría (periódicos) |
| `nehuentue/{clientId}/status` | Estado del sistema (periódico) |

Ejemplo: `nehuentue/nehuentue_sensor_001/cmd`

---

## 📋 Comandos Disponibles

### 1️⃣ Obtener Estado del Sistema

**Comando:**
```json
{"cmd":"get_status"}
```

**Respuesta:**
```json
{
  "cmd": "get_status",
  "status": "ok",
  "system": {
    "uptime": 3600,
    "heap_free": 201724,
    "cpu_freq": 160
  },
  "wifi": {
    "connected": true,
    "ssid": "Amanda 2.4G",
    "rssi": -45,
    "ip": "192.168.1.150"
  },
  "mqtt": {
    "connected": true,
    "server": "192.168.1.100"
  },
  "modbus": {
    "enabled": true,
    "reads_ok": 1250,
    "reads_fail": 3
  }
}
```

---

### 2️⃣ Obtener Configuración Actual

**Comando:**
```json
{"cmd":"get_config"}
```

**Respuesta:**
```json
{
  "cmd": "get_config",
  "status": "ok",
  "wifi": {
    "ssid": "Amanda 2.4G",
    "hostname": "Nehuentue-Sensor"
  },
  "mqtt": {
    "server": "192.168.1.100",
    "port": 1883,
    "user": "mqttuser",
    "client_id": "nehuentue_sensor_001"
  },
  "sensor": {
    "name": "Sensor 1",
    "address": 1,
    "register": 0,
    "count": 10
  }
}
```

---

### 3️⃣ Configurar WiFi

**Comando:**
```json
{
  "cmd": "set_wifi",
  "ssid": "NuevaRed",
  "password": "nuevacontraseña"
}
```

**Respuesta:**
```json
{
  "status": "ok",
  "message": "WiFi guardado, reinicia para aplicar"
}
```

**Nota:** Requiere reiniciar el dispositivo para aplicar cambios.

---

### 4️⃣ Configurar MQTT

**Comando:**
```json
{
  "cmd": "set_mqtt",
  "server": "192.168.1.200",
  "port": 1883,
  "user": "nuevo_usuario",
  "password": "nueva_pass"
}
```

**Respuesta:**
```json
{
  "status": "ok",
  "message": "MQTT guardado, reinicia para aplicar"
}
```

**Nota:** Requiere reiniciar el dispositivo para aplicar cambios.

---

### 5️⃣ Configurar Sensor Modbus

**Comando:**
```json
{
  "cmd": "set_sensor",
  "name": "Temperatura",
  "address": 1,
  "register": 0,
  "count": 2,
  "multiplier": 0.1
}
```

**Parámetros:**
- `name`: Nombre del sensor
- `address`: Dirección Modbus del esclavo (1-247)
- `register`: Registro de inicio
- `count`: Cantidad de registros a leer
- `multiplier`: Multiplicador para conversión (opcional)

**Respuesta:**
```json
{
  "status": "ok",
  "message": "Sensor configurado"
}
```

---

### 6️⃣ Escanear Redes WiFi

**Comando:**
```json
{"cmd":"scan_wifi"}
```

**Respuesta:**
```json
{
  "cmd": "scan_wifi",
  "status": "ok",
  "networks": [
    {
      "ssid": "Amanda 2.4G",
      "rssi": -45,
      "channel": 6,
      "encrypted": true
    },
    {
      "ssid": "Vecino_WiFi",
      "rssi": -72,
      "channel": 11,
      "encrypted": true
    }
  ]
}
```

**Nota:** Puede tardar hasta 10 segundos.

---

### 7️⃣ Reiniciar Dispositivo

**Comando:**
```json
{"cmd":"restart"}
```

**Respuesta:**
```json
{
  "status": "restarting"
}
```

**Nota:** El dispositivo se reiniciará en 1 segundo.

---

### 8️⃣ Factory Reset

**Comando:**
```json
{"cmd":"factory_reset"}
```

**Respuesta:**
```json
{
  "status": "factory_reset"
}
```

**⚠️ ADVERTENCIA:** Borra toda la configuración guardada y reinicia el dispositivo con valores predeterminados.

---

## 🔄 Comandos Simples (Retrocompatibilidad)

También se aceptan comandos simples sin JSON:

```
restart          → Reinicia el dispositivo
status           → Muestra estado en Serial Monitor
```

---

## 📡 Ejemplo de Uso con Mosquitto

### Publicar comando
```bash
mosquitto_pub -h 192.168.1.100 -u mqttuser -P 1234 \
  -t "nehuentue/nehuentue_sensor_001/cmd" \
  -m '{"cmd":"get_status"}'
```

### Suscribirse a respuestas
```bash
mosquitto_sub -h 192.168.1.100 -u mqttuser -P 1234 \
  -t "nehuentue/nehuentue_sensor_001/response"
```

### Suscribirse a telemetría
```bash
mosquitto_sub -h 192.168.1.100 -u mqttuser -P 1234 \
  -t "nehuentue/nehuentue_sensor_001/#"
```

---

## 🛠️ Solución de Problemas

### El dispositivo no responde
1. Verificar conexión WiFi: LED debe estar encendido fijo
2. Verificar conexión MQTT: Ver logs en Serial Monitor (115200 baud)
3. Verificar tópico correcto: Usar el `clientId` configurado

### No se conecta a WiFi
1. Verificar credenciales en `config.h`
2. Verificar que el router esté en 2.4GHz (ESP32-C3 no soporta 5GHz)
3. Reiniciar el dispositivo

### No se conecta a MQTT
1. Verificar que el broker esté accesible: `ping 192.168.1.100`
2. Verificar credenciales MQTT
3. Verificar que el broker acepte conexiones remotas
4. Usar comando `get_status` vía Serial para debug

---

## 📝 Notas

- **Formato JSON**: Todos los comandos deben ser JSON válido
- **Tiempo de espera**: El dispositivo responde en menos de 1 segundo
- **Persistencia**: Configuraciones se guardan en Flash automáticamente
- **Seguridad**: Cambiar credenciales por defecto en producción

---

## 🎯 Cambios en v2.1

- ✅ Eliminado servidor web AsyncWebServer (ahorro de 165KB Flash)
- ✅ Configuración 100% por MQTT
- ✅ Credenciales WiFi/MQTT preconfiguradas
- ✅ Sin problemas de Task Watchdog
- ✅ Sistema más estable y ligero
