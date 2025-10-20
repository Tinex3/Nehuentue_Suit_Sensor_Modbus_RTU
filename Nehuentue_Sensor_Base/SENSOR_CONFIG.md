# 🔧 Sistema de Configuración Dinámica de Sensores (v2.0)

## 🎯 **OVERVIEW**

El sistema permite **reconfigurar completamente el sensor y sus comandos Modbus sin recompilar el firmware**. Ahora no solo puedes cambiar cómo se interpreta el dato (multiplicador/offset), sino también **qué comando Modbus se ejecuta** (función, dirección, cantidad de registros).

**✨ NOVEDAD v2.0:** Configuración completa de comandos Modbus (función, dirección, registros)

**🌐 PRÓXIMAMENTE:** Interfaz web embebida para configuración sin comandos MQTT. Ver [`WEB_CONFIG.md`](WEB_CONFIG.md) para más detalles.

---

## 📱 **MÉTODOS DE CONFIGURACIÓN**

| Método | Estado | Descripción |
|--------|--------|-------------|
| **MQTT** | ✅ Disponible | Enviar comandos JSON vía tópico `cmd/sensor_config` |
| **Web UI** | 🔜 En desarrollo | Interfaz web en `http://192.168.x.x` (ver [`WEB_CONFIG.md`](WEB_CONFIG.md)) |
| **Serial** | ⏳ Futuro | Comandos por puerto serie USB |

---

## 📊 **TIPOS DE SENSORES SOPORTADOS**

El sistema es **agnóstico** y soporta cualquier tipo de sensor Modbus RTU. Ejemplos:

| Tipo | Unidades Típicas | Ejemplos de Uso |
|------|------------------|-----------------|
| `temperature` | celsius, fahrenheit, kelvin | Termómetros, sondas térmicas |
| `humidity` | percent | Higrómetros, sensores ambientales |
| `energy` | kWh, Wh, MWh | Medidores de energía eléctrica |
| `power` | W, kW, MW | Vatímetros, medidores de potencia |
| `voltage` | V, mV, kV | Voltímetros |
| `current` | A, mA | Amperímetros |
| `flow` | m3/h, L/min, gpm | Caudalímetros, flujómetros |
| `pressure` | bar, psi, Pa, kPa | Manómetros, transductores de presión |
| `level` | m, cm, mm, % | Sensores de nivel de tanques |
| `speed` | rpm, Hz | Tacómetros, encoders |
| `weight` | kg, g, ton | Balanzas, celdas de carga |
| **CUALQUIER OTRO** | Configurable | Sistema totalmente flexible |

---

## 🏗️ **ESTRUCTURA DE CONFIGURACIÓN**

### **SensorConfig** (hasta 4 sensores simultáneos)

```cpp
struct SensorConfig {
    char name[32];            // Nombre identificador (ej: "temp_sala_1")
    char type[32];            // Tipo de sensor (ej: "temperature", "energy")
    char unit[16];            // Unidad de medida (ej: "celsius", "kWh")
    
    // 🆕 PARÁMETROS MODBUS CONFIGURABLES
    uint8_t modbusAddress;    // Dirección Modbus del esclavo (1-247)
    uint8_t modbusFunction;   // Función Modbus (0x03, 0x04, etc.)
    uint16_t registerStart;   // Registro/dirección inicial
    uint16_t registerCount;   // Cantidad de registros a leer (1-4)
    
    // Conversión de datos
    float multiplier;         // Multiplicador de conversión
    float offset;             // Offset de conversión
    uint8_t decimals;         // Decimales a mostrar (0-4)
    bool enabled;             // Habilitado/Deshabilitado
};
```

### **Funciones Modbus Soportadas**

| Código | Nombre | Descripción |
|--------|--------|-------------|
| `0x03` (3) | Read Holding Registers | Lee registros de retención (lectura/escritura) |
| `0x04` (4) | Read Input Registers | Lee registros de entrada (solo lectura) |
| `0x01` (1) | Read Coils | Lee bobinas (bits) - Próximamente |
| `0x02` (2) | Read Discrete Inputs | Lee entradas discretas (bits) - Próximamente |

### **Fórmula de Conversión**

```
Valor Final = (Valor Raw Modbus × multiplier) + offset
```

**Ejemplos:**
- **Temperatura dividida por 10:** `multiplier = 0.1`, `offset = 0`
  - Raw: `253` → Final: `25.3 °C`
- **Kelvin a Celsius:** `multiplier = 0.1`, `offset = -273.15`
  - Raw: `2981` → Final: `24.95 °C`
- **Energía en Wh a kWh:** `multiplier = 0.001`, `offset = 0`
  - Raw: `12500` → Final: `12.5 kWh`

---

## 📡 **COMANDO MQTT: sensor_config**

### **Tópico**
```
devices/{deviceId}/cmd/sensor_config
```

### **Payload JSON Completo (v2.0)**
```json
{
  "sensor_id": 0,
  "type": "energy",
  "unit": "kWh",
  "multiplier": 0.001,
  "offset": 0,
  "decimals": 3,
  "modbus_function": 4,
  "start_address": 4096,
  "register_count": 2
}
```

### **Parámetros**

| Campo | Tipo | Obligatorio | Descripción |
|-------|------|-------------|-------------|
| `sensor_id` | int | ✅ | ID del sensor (0-3) |
| `type` | string | ✅ | Tipo de sensor (temperature, energy, flow, etc.) |
| `unit` | string | ✅ | Unidad de medida (celsius, kWh, m3/h, etc.) |
| `multiplier` | float | ✅ | Multiplicador para conversión |
| `offset` | float | ✅ | Offset para conversión |
| `decimals` | int | ✅ | Decimales a mostrar (0-4) |
| 🆕 `modbus_function` | int | ⚠️ Opcional* | Función Modbus (3 o 4, default: 3) |
| 🆕 `start_address` | int | ⚠️ Opcional* | Registro inicial (0-65535, default: sensor_id) |
| 🆕 `register_count` | int | ⚠️ Opcional* | Cantidad de registros (1-4, default: 1) |

> **\*Opcional:** Si se omiten los parámetros Modbus, se mantienen los valores actuales del sensor.

---

## 🔥 **EJEMPLOS DE USO**

### **1. Configurar Sensor de Temperatura (Básico)**

```bash
mosquitto_pub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/cmd/sensor_config" \
  -m '{"sensor_id":0,"type":"temperature","unit":"celsius","multiplier":0.1,"offset":0,"decimals":2}'
```

**Resultado:**
- Lee registro Modbus 0
- Si raw = 253 → publica 25.30 °C

---

### **2. Configurar Medidor de Energía**

```bash
mosquitto_pub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/cmd/sensor_config" \
  -m '{"sensor_id":0,"type":"energy","unit":"kWh","multiplier":0.001,"offset":0,"decimals":3}'
```

**Resultado:**
- Lee registro Modbus 0
- Si raw = 12500 → publica 12.500 kWh

---

### **3. Configurar Caudalímetro (Flujo de Agua)**

```bash
mosquitto_pub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/cmd/sensor_config" \
  -m '{"sensor_id":0,"type":"flow","unit":"m3/h","multiplier":0.01,"offset":0,"decimals":2}'
```

**Resultado:**
- Lee registro Modbus 0
- Si raw = 350 → publica 3.50 m³/h

---

### **4. Configurar Manómetro (Presión)**

```bash
mosquitto_pub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/cmd/sensor_config" \
  -m '{"sensor_id":0,"type":"pressure","unit":"bar","multiplier":0.01,"offset":0,"decimals":2}'
```

**Resultado:**
- Lee registro Modbus 0
- Si raw = 155 → publica 1.55 bar

---

### **5. Configurar Voltímetro**

```bash
mosquitto_pub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/cmd/sensor_config" \
  -m '{"sensor_id":0,"type":"voltage","unit":"V","multiplier":0.1,"offset":0,"decimals":1}'
```

**Resultado:**
- Lee registro Modbus 0
- Si raw = 2200 → publica 220.0 V

---

### **6. Configurar Amperímetro**

```bash
mosquitto_pub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/cmd/sensor_config" \
  -m '{"sensor_id":0,"type":"current","unit":"A","multiplier":0.01,"offset":0,"decimals":2}'
```

**Resultado:**
- Lee registro Modbus 0
- Si raw = 1550 → publica 15.50 A

---

### **7. Configurar Tacómetro (RPM)**

```bash
mosquitto_pub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/cmd/sensor_config" \
  -m '{"sensor_id":0,"type":"speed","unit":"rpm","multiplier":1,"offset":0,"decimals":0}'
```

**Resultado:**
- Lee registro Modbus 0
- Si raw = 1500 → publica 1500 rpm

---

### **8. Configurar Sensor de Nivel (%)**

```bash
mosquitto_pub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/cmd/sensor_config" \
  -m '{"sensor_id":0,"type":"level","unit":"percent","multiplier":0.1,"offset":0,"decimals":1}'
```

**Resultado:**
- Lee registro Modbus 0
- Si raw = 755 → publica 75.5%

---

## 📤 **PAYLOAD DE TELEMETRÍA RESULTANTE**

Después de configurar el sensor, el payload MQTT publicado será:

```json
{
  "device_id": "modbus-01",
  "device_type": "modbus_sensor",
  "timestamp": 1729382400,
  "datetime": "2025-10-19T15:30:00-03:00",
  "sensor": {
    "name": "energy",
    "type": "energy",
    "value": "12.500",
    "unit": "kWh",
    "modbus_address": 1,
    "register": 0
  },
  "meta": {
    "uptime": 12345,
    "rssi": -65,
    "firmware": "1.0.0",
    "power_source": "USB"
  }
}
```

---

## 🔄 **CONFIGURACIÓN POR DEFECTO**

Al iniciar, el sistema carga esta configuración:

```cpp
// Sensor 0: Temperatura
{
  "name": "temperature",
  "type": "temperature",
  "unit": "celsius",
  "modbusAddress": 1,
  "modbusFunction": 0x03,  // Read Holding Registers
  "registerStart": 0,
  "registerCount": 1,
  "multiplier": 0.1,
  "offset": 0.0,
  "decimals": 2,
  "enabled": true
}

// Sensor 1: Humedad
{
  "name": "humidity",
  "type": "humidity",
  "unit": "percent",
  "modbusAddress": 1,
  "modbusFunction": 0x03,  // Read Holding Registers
  "registerStart": 1,
  "registerCount": 1,
  "multiplier": 0.1,
  "offset": 0.0,
  "decimals": 2,
  "enabled": true
}
```

---

## 🚀 **EJEMPLOS AVANZADOS (v2.0) - Configuración Completa de Modbus**

### **A) Medidor de Energía SDM120 (Función 0x04, Dirección 0x1000)**

Este medidor guarda la energía en registros de entrada (input registers) en la dirección 0x1000 (4096 decimal):

```bash
mosquitto_pub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/cmd/sensor_config" \
  -m '{"sensor_id":0,"type":"energy","unit":"kWh","multiplier":0.001,"offset":0,"decimals":3,"modbus_function":4,"start_address":4096,"register_count":2}'
```

**¿Qué hace?**
- `modbus_function: 4` → Usa función 0x04 (Read Input Registers)
- `start_address: 4096` → Lee desde dirección 0x1000
- `register_count: 2` → Lee 2 registros (32 bits = float)

---

### **B) Caudalímetro en Esclavo Modbus ID=5, Registro 0x0010**

Flujómetro conectado con dirección Modbus 5, datos en registro 0x0010 (16 decimal):

```bash
mosquitto_pub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/cmd/sensor_config" \
  -m '{"sensor_id":0,"type":"flow","unit":"m3/h","multiplier":0.01,"offset":0,"decimals":2,"modbus_function":3,"start_address":16,"register_count":1}'
```

**¿Qué hace?**
- Cambia a esclavo Modbus ID=1 (si necesitas cambiar ID, actualiza `modbusAddress` en el código)
- `modbus_function: 3` → Función 0x03 (Read Holding Registers)
- `start_address: 16` → Lee desde dirección 0x0010
- `register_count: 1` → Lee 1 registro (16 bits)

---

### **C) Manómetro en Registro 0x0005, Función 0x03**

Sensor de presión en registro 0x0005 (5 decimal):

```bash
mosquitto_pub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/cmd/sensor_config" \
  -m '{"sensor_id":1,"type":"pressure","unit":"bar","multiplier":0.01,"offset":0,"decimals":2,"modbus_function":3,"start_address":5,"register_count":1}'
```

**¿Qué hace?**
- Configura el **segundo sensor** (sensor_id: 1)
- Lee desde registro 0x0005
- Función 0x03 (Holding Registers)

---

### **D) Sensor de Temperatura con Input Registers (0x04)**

Algunos sensores usan Input Registers en lugar de Holding Registers:

```bash
mosquitto_pub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/cmd/sensor_config" \
  -m '{"sensor_id":0,"type":"temperature","unit":"celsius","multiplier":0.1,"offset":0,"decimals":2,"modbus_function":4,"start_address":0,"register_count":1}'
```

**¿Qué hace?**
- `modbus_function: 4` → Función 0x04 (Read Input Registers) en lugar de 0x03

---

### **E) Voltímetro en Registro Alto (0x2000 = 8192 decimal)**

Algunos dispositivos usan direcciones altas:

```bash
mosquitto_pub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/cmd/sensor_config" \
  -m '{"sensor_id":0,"type":"voltage","unit":"V","multiplier":0.1,"offset":0,"decimals":1,"modbus_function":3,"start_address":8192,"register_count":1}'
```

**¿Qué hace?**
- Lee desde registro 0x2000 (8192 decimal)
- Útil para dispositivos con mapas de registros complejos

---

## 🔢 **MÚLTIPLES SENSORES**

Puedes configurar hasta **4 sensores simultáneos** (sensor_id: 0-3):

```bash
# Sensor 0: Voltaje
mosquitto_pub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/cmd/sensor_config" \
  -m '{"sensor_id":0,"type":"voltage","unit":"V","multiplier":0.1,"offset":0,"decimals":1}'

# Sensor 1: Corriente
mosquitto_pub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/cmd/sensor_config" \
  -m '{"sensor_id":1,"type":"current","unit":"A","multiplier":0.01,"offset":0,"decimals":2}'

# Sensor 2: Potencia
mosquitto_pub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/cmd/sensor_config" \
  -m '{"sensor_id":2,"type":"power","unit":"W","multiplier":1,"offset":0,"decimals":0}'

# Sensor 3: Energía
mosquitto_pub -h 192.168.1.25 -u mqttuser -P 1234 \
  -t "devices/modbus-01/cmd/sensor_config" \
  -m '{"sensor_id":3,"type":"energy","unit":"kWh","multiplier":0.001,"offset":0,"decimals":3}'
```

**El sistema publicará 4 mensajes MQTT separados, uno por cada sensor.**

---

## 💾 **PERSISTENCIA (TODO)**

⚠️ **Actualmente la configuración se pierde al reiniciar.**

**Próxima versión:** Los cambios se guardarán en EEPROM automáticamente.

---

## 🔍 **VERIFICACIÓN**

Cuando cambias la configuración, verás en el monitor serial:

```
[MQTT] Mensaje recibido:
  Tópico: devices/modbus-01/cmd/sensor_config
  Payload: {"sensor_id":0,"type":"energy","unit":"kWh",...}
  🔧 Comando de configuración de sensores recibido
  ✅ Sensor 0 reconfigurado:
     Tipo: energy
     Unidad: kWh
     Multiplicador: 0.0010
     Offset: 0.00
     Decimales: 3
```

---

## 🚀 **CASOS DE USO REALES**

### **Caso 1: Monitor de Energía Trifásico**
```bash
# Fase R - Voltaje
mosquitto_pub ... -m '{"sensor_id":0,"type":"voltage","unit":"V","multiplier":0.1,"offset":0,"decimals":1}'

# Fase S - Voltaje
mosquitto_pub ... -m '{"sensor_id":1,"type":"voltage","unit":"V","multiplier":0.1,"offset":0,"decimals":1}'

# Fase T - Voltaje
mosquitto_pub ... -m '{"sensor_id":2,"type":"voltage","unit":"V","multiplier":0.1,"offset":0,"decimals":1}'
```

### **Caso 2: Sistema de Riego**
```bash
# Sensor de flujo
mosquitto_pub ... -m '{"sensor_id":0,"type":"flow","unit":"L/min","multiplier":0.1,"offset":0,"decimals":1}'

# Sensor de presión
mosquitto_pub ... -m '{"sensor_id":1,"type":"pressure","unit":"bar","multiplier":0.01,"offset":0,"decimals":2}'
```

### **Caso 3: Monitoreo Ambiental**
```bash
# Temperatura
mosquitto_pub ... -m '{"sensor_id":0,"type":"temperature","unit":"celsius","multiplier":0.1,"offset":0,"decimals":2}'

# Humedad
mosquitto_pub ... -m '{"sensor_id":1,"type":"humidity","unit":"percent","multiplier":0.1,"offset":0,"decimals":1}'
```

---

## 🎯 **VENTAJAS**

✅ **Sin recompilar firmware** - Cambios en tiempo real  
✅ **Totalmente flexible** - Cualquier tipo de sensor  
✅ **Fácil despliegue** - Mismo firmware para todos los dispositivos  
✅ **Escalable** - Hasta 4 sensores por dispositivo  
✅ **Conversión automática** - Fórmulas configurables  
✅ **Unidades personalizadas** - Define tus propias unidades  

---

## 📌 **PRÓXIMAS MEJORAS**

- [ ] Persistencia en EEPROM (sobrevivir reinicios)
- [ ] Más de 4 sensores (límite actual)
- [ ] Soporte para registros de 32 bits (float Modbus)
- [ ] Nombres de sensores personalizados
- [ ] Validación de configuración (rangos válidos)
- [ ] API REST para configuración web
- [ ] Actualización OTA (Over-The-Air)

---

## 🛠️ **TROUBLESHOOTING**

### **Problema: Valor incorrecto después de configurar**
**Solución:** Verifica el `multiplier` y `offset`. Usa la fórmula:
```
Valor Final = (Raw × multiplier) + offset
```

### **Problema: No publica datos**
**Solución:** Asegúrate de que `enabled = true` y que el sensor Modbus responda.

### **Problema: Configuración se pierde al reiniciar**
**Solución:** Actualmente esperado. Próxima versión guardará en EEPROM automáticamente.

---

**Última actualización:** 19 de octubre de 2025  
**Versión del firmware:** 1.0.0  
**Estado:** ✅ **FUNCIONAL - LISTO PARA USAR**
