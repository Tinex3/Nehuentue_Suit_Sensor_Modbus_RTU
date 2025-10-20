# Changelog - Gateway Modbus-MQTT ESP32

## [2.0.0] - 19 de Octubre de 2025

### 🎯 FEATURE PRINCIPAL: Configuración Modbus Dinámica Completa

Sistema completamente reconfigurable sin necesidad de recompilar firmware. Ahora es posible cambiar no solo la interpretación de datos, sino también **qué comando Modbus se ejecuta**.

#### ✨ Agregado

- **Parámetros Modbus configurables vía MQTT**:
  - `modbus_function`: Función Modbus (0x03, 0x04, etc.)
  - `start_address`: Registro inicial a leer
  - `register_count`: Cantidad de registros (1-4)
  
- **modbusTask() dinámico**:
  - Loop que itera sobre `sensorsConfig.sensors[]`
  - Ejecuta comandos Modbus según configuración de cada sensor
  - Soporte para funciones 0x03 (Holding) y 0x04 (Input)
  - Preparado para 0x01 (Coils) y 0x02 (Discrete)
  
- **decoderTask() flexible**:
  - Procesa de 1 a 4 registros dinámicamente
  - Almacena en array `registers[4]` por sensor
  - Identifica sensor origen con `sensorIndex`
  
- **Estructura de datos multi-sensor**:
  - `SingleSensorData` para almacenar hasta 4 registros
  - `SensorData.sensors[4]` para múltiples sensores simultáneos
  - Campos legacy mantenidos para compatibilidad
  
- **Documentación completa**:
  - [`WEB_CONFIG.md`](WEB_CONFIG.md): Especificación de interfaz web futura
  - [`RESUMEN_CAMBIOS_v2.0.md`](RESUMEN_CAMBIOS_v2.0.md): Resumen ejecutivo
  - [`SENSOR_CONFIG.md`](SENSOR_CONFIG.md): Actualizado con nuevos parámetros

#### 🔧 Modificado

- **SensorConfig struct**: Agregado campo `modbusFunction`
- **RawModbusData struct**: Agregado campo `sensorIndex`
- **mqttCallback()**: Extendido para parsear parámetros Modbus
- **initDefaultConfig()**: Incluye `modbusFunction = 0x03` por defecto

#### 📋 Ejemplos de Uso

**Medidor de Energía (Función 0x04, Registro 4096):**
```bash
mosquitto_pub -h 192.168.1.25 -t devices/modbus-01/cmd/sensor_config -m \
'{
  "sensor_id": 0,
  "type": "energy",
  "unit": "kWh",
  "multiplier": 0.001,
  "offset": 0,
  "decimals": 3,
  "modbus_function": 4,
  "start_address": 4096,
  "register_count": 2
}'
```

**Flujómetro (Función 0x03, Registro 16):**
```bash
mosquitto_pub -h 192.168.1.25 -t devices/modbus-01/cmd/sensor_config -m \
'{
  "sensor_id": 1,
  "type": "flow",
  "unit": "m3/h",
  "multiplier": 0.01,
  "offset": 0,
  "decimals": 2,
  "modbus_function": 3,
  "start_address": 16,
  "register_count": 1
}'
```

#### 🔜 Próximamente

- Interfaz web embebida para configuración (ver [`WEB_CONFIG.md`](WEB_CONFIG.md))
- Modo Access Point automático para primera configuración
- Persistencia de configuración Modbus en EEPROM
- Soporte para funciones Modbus 0x01, 0x02, 0x05, 0x06

---

## [1.5.0] - Octubre 2025

### Resumen de Cambios - EEPROM Ultra Genérica + Configuración MQTT + Arquitectura de Tópicos

## ✅ Cambios Implementados

### 1. EEPROM Ultra Genérica - Tamaño Configurable

**Archivo: `include/eeprom_manager.h`**
- ✅ Agregado miembro privado `uint16_t eepromSize`
- ✅ Modificado `begin()` para aceptar parámetro `size`:
  ```cpp
  EEPROMStatus begin(int sdaPin, int sclPin, 
                     uint16_t size = EEPROM_SIZE,  // Nuevo parámetro
                     uint32_t frequency = I2C_MASTER_FREQ_HZ, 
                     uint8_t deviceAddr = EEPROM_I2C_ADDRESS);
  ```
- ✅ Agregada función `uint16_t getSize()`
- ✅ Todas las verificaciones de límites ahora usan `eepromSize` en vez de `EEPROM_SIZE`
- ✅ Compatible con 24LC32, 24LC64, **24LC128**, 24LC256, 24LC512

**Archivo: `src/eeprom_manager.cpp`**
- ✅ Constructor inicializa `eepromSize = EEPROM_SIZE` (16384 por defecto)
- ✅ `begin()` asigna el tamaño recibido: `eepromSize = size`
- ✅ Detección automática del modelo según tamaño
- ✅ Todas las verificaciones actualizadas (writeRaw, readRaw, clear, fill, etc.)
- ✅ Función `getSize()` implementada
- ✅ **CORREGIDO**: Eliminada función duplicada

### 2. Configuración MQTT con Autenticación

**Archivo: `include/tasks.h`**
- ✅ Agregados campos a `struct WiFiConfig`:
  ```cpp
  char mqttUser[32];       // Usuario MQTT
  char mqttPassword[64];   // Contraseña MQTT
  char deviceId[32];       // ID del dispositivo (ej: "modbus-01")
  ```
- ✅ Nueva estructura `MQTTTopics` para tópicos dinámicos
- ✅ Enum `MQTTCommand` para comandos MQTT
- ✅ Renombrada tarea `wifiMqttTask` → `mqttTask`

**Archivo: `src/tasks.cpp`**
- ✅ Configuración por defecto actualizada:
  ```cpp
  strcpy(wifiConfig.deviceId, "modbus-01");
  strcpy(wifiConfig.mqttServer, "192.168.1.25");  // IP RPi
  wifiConfig.mqttPort = 1883;
  strcpy(wifiConfig.mqttUser, "mqttuser");
  strcpy(wifiConfig.mqttPassword, "1234");
  ```
- ✅ Nueva función `buildMQTTTopics()` - Construye tópicos dinámicamente
- ✅ Tarea `mqttTask()` completamente reescrita con:
  - Gestión WiFi + MQTT unificada
  - Publicación "Hello World" para testing
  - Suscripción a comandos (`devices/modbus-01/cmd/#`)
  - Last Will (`devices/modbus-01/status`)
  - Publicación de estado cada 60s

### 3. Arquitectura de Tópicos MQTT

**Convención de tópicos**:
```
devices/{deviceId}/{categoria}/{subcategoria}
```

**Tópicos implementados**:
- ✅ `devices/modbus-01/telemetry/temperature` - Telemetría temperatura
- ✅ `devices/modbus-01/telemetry/current` - Telemetría corriente
- ✅ `devices/modbus-01/status` - Estado del dispositivo
- ✅ `devices/modbus-01/event/error` - Eventos de error
- ✅ `devices/modbus-01/cmd/#` - Recepción de comandos

### 4. Documentación

**Nuevos archivos creados:**
- ✅ `EEPROM_USAGE.md` - Guía completa de uso de la librería EEPROM
- ✅ `MQTT_CONFIG.md` - Configuración MQTT detallada
- ✅ `MQTT_TOPICS.md` - **NUEVO** Arquitectura completa de tópicos MQTT

## 📋 Ejemplos de Uso

### Inicializar EEPROM para diferentes modelos

```cpp
// 24LC64 (8 KB)
EEPROM24LC64.begin(8, 9, 8192);

// 24LC128 (16 KB) - Configuración actual
EEPROM24LC64.begin(8, 9, 16384);

// 24LC256 (32 KB)
EEPROM24LC64.begin(8, 9, 32768);

// Usar tamaño por defecto (definido en header)
EEPROM24LC64.begin(8, 9);  // Usa 16384 (24LC128)
```

### Guardar configuración MQTT

```cpp
WiFiConfig config;
strcpy(config.mqttServer, "192.168.1.25");
config.mqttPort = 1883;
strcpy(config.mqttUser, "mqttuser");
strcpy(config.mqttPassword, "1234");

// Guardar con CRC
EEPROM24LC64.saveWithCRC<WiFiConfig>(0, config);

// Cargar y verificar
WiFiConfig configLeida;
if (EEPROM24LC64.loadWithCRC<WiFiConfig>(0, configLeida) == EEPROM_OK) {
    Serial.println("Configuración válida");
}
```

## 🔧 Configuración Actual

### EEPROM
- **Modelo**: 24LC128 (16 KB)
- **Dirección I2C**: 0x50
- **Pines**: SDA=GPIO8, SCL=GPIO9
- **Frecuencia**: 100 kHz
- **Tamaño configurable**: Sí ✓

### MQTT
- **Broker**: 192.168.1.25 (Raspberry Pi)
- **Puerto**: 1883
- **Usuario**: mqttuser
- **Contraseña**: 1234
- **Topic**: sensor/nehuentue

## 🐛 Problemas Corregidos

1. ✅ **Función `getSize()` duplicada** - Eliminada duplicación en línea 403
2. ✅ **EEPROM_SIZE hardcodeado** - Ahora usa variable `eepromSize` configurable
3. ✅ **Falta autenticación MQTT** - Agregados campos user/password
4. ✅ **Configuración MQTT sin persistencia** - Ahora se guarda en EEPROM con CRC

## 📊 Mapa de Memoria EEPROM

```
Dirección  | Contenido                    | Tamaño
-----------|------------------------------|--------
0x0000     | WiFiConfig (con CRC)         | ~200 bytes
0x0100     | StoredSensorData (con CRC)   | ~20 bytes
0x0200-... | Disponible                   | ~15.8 KB
```

## ⚙️ Próximos Pasos

Para activar WiFi/MQTT, descomentar en `src/tasks.cpp`:

```cpp
#include <WiFi.h>
#include <PubSubClient.h>
```

Y agregar a `platformio.ini`:

```ini
lib_deps = 
    knolleary/PubSubClient@^2.8
```

## 🧪 Testing

### Verificar tamaño EEPROM

```cpp
Serial.printf("Tamaño EEPROM: %d bytes\n", EEPROM24LC64.getTotalSize());
// Esperado: 16384 (24LC128)
```

### Verificar configuración MQTT

```cpp
EEPROM24LC64.printStatus();
// Mostrará modelo detectado automáticamente
```

### Ver memoria EEPROM

```cpp
EEPROM24LC64.dumpMemory(0, 256);  // Primeros 256 bytes
```

## 📝 Notas Importantes

1. **Compatibilidad**: La librería funciona con TODA la familia 24LCXX sin cambios de código
2. **Thread-Safe**: Todas las operaciones usan mutex, seguras para FreeRTOS
3. **CRC16**: Verificación automática de integridad opcional
4. **No-Blocking**: Usa driver ESP32 nativo con timeouts
5. **Migración fácil**: Solo cambia el parámetro `size` en `begin()`

## ✨ Ventajas de la Implementación

- ✅ Ultra genérica - No depende de estructuras específicas
- ✅ Configurable en runtime - Tamaño de EEPROM por parámetro
- ✅ Persistencia MQTT - Configuración sobrevive a reinicios
- ✅ Verificación CRC - Detecta datos corruptos
- ✅ Auto-detección - Muestra modelo según tamaño
- ✅ Documentación completa - Guías de uso y ejemplos
