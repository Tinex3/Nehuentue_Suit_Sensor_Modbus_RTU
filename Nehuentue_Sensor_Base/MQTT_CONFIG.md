# Configuración MQTT para Nehuentue Sensor Base

## Datos de conexión MQTT

La configuración predeterminada del sistema usa los siguientes parámetros para conectarse al broker MQTT:

```cpp
MQTT Server:   192.168.1.25  // IP de la Raspberry Pi
MQTT Port:     1883           // Puerto estándar MQTT
MQTT User:     mqttuser       // Usuario de autenticación
MQTT Password: 1234           // Contraseña de autenticación
MQTT Topic:    sensor/nehuentue
```

## Cómo funciona

### 1. Almacenamiento en EEPROM

La configuración WiFi/MQTT se guarda automáticamente en la EEPROM con verificación CRC16:

```cpp
struct WiFiConfig {
    char ssid[32];
    char password[64];
    char mqttServer[64];
    uint16_t mqttPort;
    char mqttUser[32];
    char mqttPassword[64];
    char mqttTopic[64];
};
```

**Direcciones de memoria EEPROM:**
- Configuración WiFi/MQTT: `0x0000` (con CRC)
- Últimos datos del sensor: `0x0100` (con CRC)

### 2. Inicialización automática

Al arrancar, el sistema:

1. **Intenta cargar** la configuración desde EEPROM con verificación CRC
2. **Si no existe o CRC es inválido**, usa los valores por defecto (los de arriba)
3. **Guarda** la configuración por defecto en EEPROM para la próxima vez

### 3. Conexión MQTT

La tarea WiFi/MQTT realiza:

```cpp
// Conecta con autenticación
mqttClient.connect("ESP32_SensorBase", mqttUser, mqttPassword);

// Publica en el topic configurado
mqttClient.publish(mqttTopic, jsonPayload);
```

**Payload JSON publicado cada 10 segundos:**
```json
{
  "temperature": 23.5,
  "humidity": 65.2,
  "timestamp": 1234567890
}
```

## Cambiar la configuración

### Opción 1: Modificar valores por defecto

Edita `src/tasks.cpp` en la función `eepromTask()`:

```cpp
strcpy(wifiConfig.ssid, "TuSSID");
strcpy(wifiConfig.password, "TuPassword");
strcpy(wifiConfig.mqttServer, "192.168.1.25");
wifiConfig.mqttPort = 1883;
strcpy(wifiConfig.mqttUser, "mqttuser");
strcpy(wifiConfig.mqttPassword, "1234");
strcpy(wifiConfig.mqttTopic, "sensor/nehuentue");
```

### Opción 2: Actualizar en runtime (TODO)

Puedes agregar comandos Serial para actualizar la configuración:

```cpp
// Ejemplo de comando serial
if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    
    if (cmd.startsWith("MQTT_SERVER:")) {
        strcpy(wifiConfig.mqttServer, cmd.substring(12).c_str());
        EEPROM24LC64.saveWithCRC<WiFiConfig>(EEPROM_ADDR_WIFI_CONFIG, wifiConfig);
        Serial.println("Servidor MQTT actualizado");
    }
}
```

### Opción 3: Borrar EEPROM y reiniciar

Si quieres forzar que use los valores por defecto:

```cpp
// En setup() o por comando serial
EEPROM24LC64.clearAll();
ESP.restart();
```

## Verificación de conectividad

### Monitor Serial

El sistema muestra logs detallados:

```
[EEPROM TASK] Cargando configuración WiFi...
[EEPROM TASK] ✓ Configuración WiFi cargada con CRC válido
  SSID: MiWiFi
  MQTT Server: 192.168.1.25:1883
  MQTT User: mqttuser

[WIFI/MQTT TASK] Conectando a WiFi...
[WIFI/MQTT TASK] ✓ WiFi conectado
  IP: 192.168.1.123

[WIFI/MQTT TASK] ✓ MQTT conectado
  Broker: 192.168.1.25:1883
  Usuario: mqttuser

[WIFI/MQTT TASK] Payload: {"temperature":23.5,"humidity":65.2,"timestamp":1234567890}
[WIFI/MQTT TASK] ✓ Datos publicados
```

### Estados de error MQTT

Si ves errores MQTT, comprueba:

```
Error -4: MQTT_CONNECTION_TIMEOUT
Error -3: MQTT_CONNECTION_LOST
Error -2: MQTT_CONNECT_FAILED
Error -1: MQTT_DISCONNECTED
Error  1: MQTT_CONNECT_BAD_PROTOCOL
Error  2: MQTT_CONNECT_BAD_CLIENT_ID
Error  3: MQTT_CONNECT_UNAVAILABLE
Error  4: MQTT_CONNECT_BAD_CREDENTIALS  <- Usuario/contraseña incorrectos
Error  5: MQTT_CONNECT_UNAUTHORIZED
```

## Suscripción a topics desde Raspberry Pi

Para monitorear los datos desde tu RPi:

```bash
# Suscribirse al topic
mosquitto_sub -h localhost -p 1883 -u mqttuser -P 1234 -t "sensor/nehuentue" -v

# Ver todos los topics
mosquitto_sub -h localhost -p 1883 -u mqttuser -P 1234 -t "#" -v
```

## Seguridad

### Recomendaciones:

1. **Cambiar contraseñas**: Los valores por defecto son inseguros
2. **Usar TLS**: Para producción, considera MQTT sobre TLS (puerto 8883)
3. **Firewall**: Asegura que el puerto 1883 solo sea accesible en tu red local
4. **Contraseñas fuertes**: No uses "1234" en producción

### Configurar Mosquitto en RPi con autenticación

```bash
# Crear archivo de contraseñas
sudo mosquitto_passwd -c /etc/mosquitto/passwd mqttuser

# Editar configuración
sudo nano /etc/mosquitto/mosquitto.conf
```

Agregar:
```
listener 1883
allow_anonymous false
password_file /etc/mosquitto/passwd
```

```bash
# Reiniciar Mosquitto
sudo systemctl restart mosquitto
```

## Librerías necesarias

Asegúrate de tener estas librerías en `platformio.ini`:

```ini
lib_deps = 
    knolleary/PubSubClient@^2.8
```

Y descomentar las líneas en `tasks.cpp`:

```cpp
#include <WiFi.h>
#include <PubSubClient.h>
```

## Formato de datos

### Payload completo esperado

```json
{
  "device": "ESP32_SensorBase",
  "temperature": 23.5,
  "humidity": 65.2,
  "timestamp": 1234567890,
  "uptime": 3600,
  "rssi": -67
}
```

Para implementar este formato extendido, modifica `wifiMqttTask()` en `tasks.cpp`.
