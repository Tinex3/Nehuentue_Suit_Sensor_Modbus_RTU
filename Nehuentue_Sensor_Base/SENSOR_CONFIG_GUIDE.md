# 🔌 Guía de Configuración del Sensor Modbus

## 📝 Resumen Rápido

**Problema resuelto:** La interfaz web ahora funciona correctamente con la arquitectura de **1 sensor con hasta 125 registros**.

**Versión:** v2.1 (19 oct 2025)

---

## 🚀 Cómo Configurar el Sensor

### **Paso 1: Conectar al Dispositivo**

#### Opción A - Primera vez (Modo AP)
1. Busca la red WiFi: `Modbus-Config-XXXX`
2. Contraseña: `modbus2024`
3. Abre el navegador: `http://192.168.4.1`

#### Opción B - Ya configurado (Modo Estación)
1. Conecta a la misma red que el ESP32
2. Abre: `http://[IP_DEL_ESP32]`

### **Paso 2: Ir a la Página de Sensores**

Dashboard → **[Sensores]**

---

## 📋 Campos del Formulario

### **Información Básica**

| Campo | Descripción | Ejemplo |
|-------|-------------|---------|
| **Nombre del Sensor** | Identificador descriptivo | `Temperatura Ambiente` |
| **Tipo** | Categoría (visual) | `temperature`, `humidity`, `custom` |
| **Unidad** | Unidad de medida | `°C`, `%`, `kWh`, `bar` |

### **Configuración Modbus**

| Campo | Descripción | Rango | Ejemplo |
|-------|-------------|-------|---------|
| **Función Modbus** | Comando de lectura | 0x03 o 0x04 | `0x03 - Read Holding Registers` |
| **Dirección Esclavo** | ID del dispositivo en bus RS485 | 1-247 | `1` |
| **Registro Inicial** | Dirección del primer registro | 0-65535 | `0` |
| **Cantidad de Registros** | Número de registros a leer | **1-125** | `10` |

> 💡 **Nuevo límite:** Ahora puedes leer **hasta 125 registros (250 bytes)** en una sola consulta.

### **Procesamiento de Datos**

| Campo | Descripción | Ejemplo |
|-------|-------------|---------|
| **Multiplicador** | Factor de escala | `1.0` (sin cambio), `0.1` (divide por 10) |
| **Offset** | Valor a sumar después de multiplicar | `0`, `-273.15` |
| **Decimales** | Precisión de visualización | `2` → `25.67°C` |

**Fórmula aplicada:**
```
Valor = (Registro_Raw × Multiplicador) + Offset
```

### **Estado**

- ☑️ **Sensor habilitado**: Si está marcado, el sensor se lee activamente

---

## 💾 Ejemplos de Configuración

### Ejemplo 1: Temperatura Simple (1 registro)
```
Nombre:             Temp Ambiente
Tipo:               temperature
Unidad:             celsius
Función Modbus:     0x03
Dirección Esclavo:  1
Registro Inicial:   0
Cantidad:           1
Multiplicador:      0.1
Offset:             0
Decimales:          1
✓ Habilitado
```
**Resultado:** Registro `256` → `25.6°C`

---

### Ejemplo 2: Energía (2 registros, float 32-bit)
```
Nombre:             Contador Energía
Tipo:               energy
Unidad:             kWh
Función Modbus:     0x04
Dirección Esclavo:  1
Registro Inicial:   100
Cantidad:           2
Multiplicador:      1.0
Offset:             0
Decimales:          2
✓ Habilitado
```

---

### Ejemplo 3: Múltiples Registros (hasta 125)
```
Nombre:             Datos Completos
Tipo:               custom
Unidad:             raw
Función Modbus:     0x03
Dirección Esclavo:  1
Registro Inicial:   0
Cantidad:           125
Multiplicador:      1.0
Offset:             0
Decimales:          0
✓ Habilitado
```

---

## 📡 Publicación MQTT

### Topic Telemetría (cada 60s)
**Topic:** `devices/modbus-01/telemetry`

```json
{
  "timestamp": "2025-10-19T21:30:00Z",
  "sensor_id": "modbus_sensor",
  "type": "temperature",
  "unit": "celsius",
  "value": 25.6,
  "raw": [256, 0, 0, 0, ...],
  "uptime": 1234
}
```

### Topic Modbus Raw (pendiente implementación)
**Topic:** `devices/modbus-01/modbus/response`

```json
{
  "timestamp": 1760919518,
  "slave_address": 1,
  "function": 3,
  "register_start": 0,
  "register_count": 10,
  "response_length": 23,
  "data": "01030A00010002000300040005C8F2"
}
```

---

## ⚠️ Limitaciones Actuales

### 🔴 **Sin Persistencia**
- La configuración **se guarda solo en RAM**
- Al reiniciar el ESP32, vuelve a valores por defecto
- **Solución planeada:** Implementar `flash_storage` con Preferences/NVS

### Configuración por Defecto (hardcoded)
```cpp
// En src/tasks.cpp - initDefaultConfig()
name = "modbus_sensor"
type = "modbus_generic"
unit = ""
modbusFunction = 0x03
modbusAddress = 1
registerStart = 0
registerCount = 10
multiplier = 1.0
offset = 0.0
decimals = 2
enabled = true
```

---

## 🛠️ Próximas Mejoras

1. ✅ **Completado:** Interfaz de 1 sensor con hasta 125 registros
2. ⏳ **Pendiente:** Sistema de persistencia (Preferences/NVS)
3. ⏳ **Pendiente:** Publicación MQTT del raw Modbus response
4. ⏳ **Pendiente:** Validación de campos en frontend

---

## 🐛 Solución de Problemas

### "No hay sensores configurados"
**Causa:** Frontend buscaba array `sensors[]` pero backend enviaba objeto `sensor{}`
**Solución:** ✅ Actualizada en v2.1

### Configuración se pierde al reiniciar
**Causa:** No hay persistencia en flash
**Solución temporal:** Reconfigurar desde web UI
**Solución permanente:** Implementar `flash_storage` (próximo paso)

### Crash/Reboot loop
**Causa:** Race condition entre tareas FreeRTOS al inicio
**Solución:** Añadidos delays iniciales en cada tarea

---

## 📞 Soporte

**Repositorio:** Nehuentue_Suit_Sensor_Modbus_RTU
**Rama:** main
**Fecha:** 19 de octubre de 2025
