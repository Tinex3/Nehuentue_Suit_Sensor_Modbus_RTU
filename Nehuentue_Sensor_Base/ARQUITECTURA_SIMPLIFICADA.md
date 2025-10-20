# 🔄 Arquitectura Simplificada - 1 Sensor, N Registros

## 📅 Fecha: 19 de octubre de 2025

## 🎯 Objetivo
Simplificar el sistema de **4 sensores** a **1 solo sensor** que pueda consultar **múltiples registros** Modbus y publicar la **respuesta cruda completa** por MQTT.

---

## 🔀 Cambios Realizados

### 1. Estructuras de Datos (`tasks.h`)

#### ✅ ANTES (4 sensores):
```cpp
#define MAX_SENSORS 4
struct SensorsConfig {
    SensorConfig sensors[MAX_SENSORS];
    uint8_t sensorCount;
};

struct SensorData {
    SingleSensorData sensors[4];
    // campos legacy...
};
```

#### ✅ DESPUÉS (1 sensor):
```cpp
// 1 solo sensor (no array)
struct SensorConfig {
    char name[32];
    char type[32];
    char unit[16];
    uint8_t modbusAddress;
    uint8_t modbusFunction;
    uint16_t registerStart;
    uint16_t registerCount;  // hasta 125 registros
    float multiplier;
    float offset;
    uint8_t decimals;
    bool enabled;
};

// Respuesta Modbus cruda completa
struct ModbusRawResponse {
    bool valid;
    uint8_t data[256];        // Respuesta completa
    uint8_t length;
    uint8_t slaveAddress;
    uint8_t functionCode;
    uint16_t registerStart;
    uint16_t registerCount;
    unsigned long timestamp;
};

// Datos del sensor
struct SensorData {
    ModbusRawResponse modbusResponse;  // Respuesta cruda
    uint16_t registers[125];           // Registros decodificados
    uint8_t registerCount;
    bool valid;
    unsigned long timestamp;
};
```

---

### 2. Variables Globales (`tasks.cpp`)

#### ✅ ANTES:
```cpp
SensorsConfig sensorsConfig;  // Array de 4 sensores
```

#### ✅ DESPUÉS:
```cpp
SensorConfig sensorConfig;    // 1 solo sensor
```

---

### 3. Tarea Modbus (`modbusTask`)

#### ✅ CAMBIOS:
- ❌ Eliminado bucle `for` sobre 4 sensores
- ✅ Lee **1 solo sensor** con `registerCount` configurable (1-125)
- ✅ Almacena **respuesta Modbus cruda completa** en `sensorData.modbusResponse`
- ✅ Logs muestran datos HEX completos

#### Código nuevo:
```cpp
void modbusTask(void *pvParameters) {
    for (;;) {
        if (!sensorConfig.enabled) {
            vTaskDelayUntil(&lastWakeTime, pollingInterval);
            continue;
        }
        
        // Lee 1 sensor con N registros
        ModbusResponse response = modbusReadHoldingRegisters(
            sensorConfig.modbusAddress,
            sensorConfig.registerStart,
            sensorConfig.registerCount  // ¡Hasta 125!
        );
        
        // Guarda respuesta CRUDA completa
        sensorData.modbusResponse.valid = response.success;
        sensorData.modbusResponse.length = response.length;
        sensorData.modbusResponse.data = response.data;  // Bytes sin procesar
        // ...
    }
}
```

---

### 4. Tarea Decoder (`decoderTask`)

#### ✅ CAMBIOS:
- ❌ Eliminado código para múltiples sensores
- ✅ Decodifica hasta **125 registros** de 1 sensor
- ✅ Muestra primeros 10 registros en logs (evita spam)

#### Código nuevo:
```cpp
void decoderTask(void *pvParameters) {
    for (;;) {
        if (sensorData.modbusResponse.valid) {
            // Extrae hasta 125 registros
            for (uint8_t i = 0; i < sensorData.modbusResponse.registerCount && i < 125; i++) {
                uint16_t reg = (data[3 + i*2] << 8) | data[4 + i*2];
                sensorData.registers[i] = reg;
            }
        }
    }
}
```

---

### 5. Tarea MQTT (`mqttTask`)

#### ✅ NUEVO TÓPICO:
```
devices/{deviceId}/modbus/response
```

#### ✅ PAYLOAD JSON:
```json
{
  "device_id": "modbus-01",
  "timestamp": 1697712000,
  "modbus": {
    "slave_address": 1,
    "function": "0x03",
    "register_start": 0,
    "register_count": 10,
    "response_length": 25,
    "data": "01 03 14 1F 40 00 64 00 C8 01 2C 01 90 01 F4 02 58 02 BC 03 20 03 84 A1 B2"
  }
}
```

#### ✅ CAMBIOS:
- ✅ Publica respuesta Modbus **cruda completa** en formato HEX
- ✅ Intervalo configurable: `mqttConfig.modbusRawInterval`
- ❌ Eliminado bucle `for` sobre sensores

---

### 6. Tópicos MQTT Actualizados

```cpp
struct MQTTTopics {
    char telemetry[64];     // devices/{deviceId}/telemetry (opcional, datos decodificados)
    char modbusRaw[64];     // devices/{deviceId}/modbus/response (NUEVO: respuesta cruda)
    char status[64];        // devices/{deviceId}/status
    char eventError[64];    // devices/{deviceId}/event/error
    char eventConnect[64];  // devices/{deviceId}/event/connect
    char cmdBase[64];       // devices/{deviceId}/cmd/#
};
```

---

### 7. Configuración MQTT

```cpp
struct MQTTConfig {
    unsigned long telemetryInterval;   // Intervalo telemetría decodificada (opcional)
    unsigned long statusInterval;      // Intervalo de estado
    unsigned long modbusRawInterval;   // Intervalo respuesta Modbus cruda (NUEVO)
};
```

**Valores por defecto:**
- `modbusRawInterval`: **5000 ms** (5 segundos)

---

## 📊 Comparación

| Aspecto | ANTES (v2.0) | DESPUÉS (v2.1 Simplificado) |
|---------|--------------|----------------------------|
| **Sensores** | 4 sensores independientes | 1 sensor con N registros |
| **Registros por sensor** | Máx 4 registros | Hasta 125 registros |
| **Publicación MQTT** | Telemetría decodificada | Respuesta Modbus cruda HEX |
| **Estructura datos** | `sensors[4]` | `ModbusRawResponse` |
| **Configuración Web** | Formulario con 4 slots | Formulario simple 1 sensor |
| **Complejidad código** | Alta (bucles anidados) | Baja (lineal) |
| **Tamaño RAM** | ~2 KB (4 sensores) | ~512 bytes (1 sensor) |

---

## ✅ Ventajas

1. **Simplicidad**: Código más limpio y fácil de mantener
2. **Flexibilidad**: Puedes leer 125 registros en 1 consulta vs 4 registros máx
3. **Debugging**: Respuesta Modbus cruda facilita troubleshooting
4. **Rendimiento**: Menos iteraciones, menor uso de RAM
5. **Escalabilidad**: Cliente MQTT puede procesar respuesta cruda como quiera

---

## 🔄 Pendientes

- [ ] Actualizar `initDefaultConfig()` para inicializar `sensorConfig` (no array)
- [ ] Actualizar `buildMQTTTopics()` para agregar `modbusRaw`
- [ ] Actualizar Web UI (`web_server.cpp`) para 1 solo sensor
- [ ] Actualizar API `/api/sensors` para POST/GET de 1 sensor
- [ ] Compilar y probar en hardware
- [ ] Actualizar documentación (README, SENSOR_CONFIG, MQTT_TOPICS)

---

## 🎯 Siguiente Paso

**Compilar código actual** para verificar errores de sintaxis y luego actualizar Web UI.
