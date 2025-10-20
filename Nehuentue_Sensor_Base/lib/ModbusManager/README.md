# 🔧 ModbusManager - Modbus RTU Master para ESP32

**Versión:** 1.0.0  
**Thread-Safe:** ✅ Sí (Mutex)  
**Arquitectura:** FreeRTOS

## 📋 Descripción

Gestor Modbus RTU Master profesional con soporte completo de funciones, estadísticas y callbacks.

## ✨ Características

- ✅ **Funciones soportadas**: 0x01, 0x03, 0x04, 0x06, 0x10
- ✅ **CRC16 automático**: Cálculo y verificación
- ✅ **Thread-safe**: Operaciones protegidas con mutex
- ✅ **Estadísticas**: Tracking completo de comunicación
- ✅ **Callbacks**: Notificaciones de respuestas
- ✅ **Detección de excepciones**: Manejo robusto de errores

## 🚀 Uso Rápido

```cpp
#include <ModbusManager.h>

void setup() {
    Serial.begin(115200);
    
    // Inicializar Modbus (Serial1, RX=GPIO4, TX=GPIO5, 9600 bps)
    ModbusMgr.begin(Serial1, 4, 5, 9600);
    
    // Leer registros del esclavo ID 1
    ModbusResponse resp = ModbusMgr.readHoldingRegisters(1, 0, 10);
    
    if (resp.success) {
        uint16_t registers[10];
        uint16_t count = ModbusMgr.extractRegisters(resp, registers, 10);
        
        for (uint16_t i = 0; i < count; i++) {
            Serial.printf("Reg[%d]: %u\n", i, registers[i]);
        }
    } else {
        Serial.println("Error en comunicación Modbus");
    }
}

void loop() {
    delay(1000);
}
```

## 📚 API Completa

### Inicialización

```cpp
bool begin(HardwareSerial& serial, int rxPin, int txPin, unsigned long baudrate = 9600);
void end();
```

### Funciones Modbus

```cpp
// Leer Holding Registers (0x03)
ModbusResponse readHoldingRegisters(uint8_t slaveId, uint16_t startAddress, uint16_t quantity);

// Leer Input Registers (0x04)
ModbusResponse readInputRegisters(uint8_t slaveId, uint16_t startAddress, uint16_t quantity);

// Leer Coils (0x01)
ModbusResponse readCoils(uint8_t slaveId, uint16_t startAddress, uint16_t quantity);

// Escribir un registro (0x06)
ModbusResponse writeSingleRegister(uint8_t slaveId, uint16_t address, uint16_t value);

// Escribir múltiples registros (0x10)
ModbusResponse writeMultipleRegisters(uint8_t slaveId, uint16_t startAddress, uint16_t quantity, uint16_t* values);
```

### Utilidades

```cpp
// Extraer registros de respuesta
uint16_t extractRegisters(const ModbusResponse& response, uint16_t* registers, size_t maxRegisters);

// Calcular CRC16
static uint16_t calculateCRC(const uint8_t* buf, size_t len);

// Verificar CRC16
static bool verifyCRC(const uint8_t* buf, size_t len);

// Descripción de excepción
static const char* getExceptionDescription(uint8_t exceptionCode);
```

### Callbacks y Configuración

```cpp
void onResponse(ModbusResponseCallback callback);
void setTimeout(uint32_t timeout);
```

### Estadísticas

```cpp
const ModbusStats& getStats();
void resetStats();
void printStats();
void printInfo();
```

## 📝 Ejemplos

### Ejemplo 1: Lectura Básica

```cpp
ModbusResponse resp = ModbusMgr.readHoldingRegisters(1, 0, 5);

if (resp.success) {
    uint16_t regs[5];
    uint16_t count = ModbusMgr.extractRegisters(resp, regs, 5);
    
    for (int i = 0; i < count; i++) {
        Serial.printf("Registro %d: %u\n", i, regs[i]);
    }
}
```

### Ejemplo 2: Escritura

```cpp
bool ok = ModbusMgr.writeSingleRegister(1, 100, 1234).success;
Serial.printf("Escritura: %s\n", ok ? "OK" : "Error");
```

### Ejemplo 3: Callback de Respuestas

```cpp
void onModbusResponse(const ModbusResponse& resp) {
    if (resp.success) {
        Serial.printf("Respuesta de esclavo %d OK\n", resp.slaveId);
    } else if (resp.exceptionCode > 0) {
        Serial.printf("Excepción: %s\n", ModbusMgr.getExceptionDescription(resp.exceptionCode));
    }
}

void setup() {
    ModbusMgr.begin(Serial1, 4, 5, 9600);
    ModbusMgr.onResponse(onModbusResponse);
}
```

### Ejemplo 4: Con FreeRTOS

```cpp
void modbusTask(void* param) {
    while (true) {
        auto resp = ModbusMgr.readHoldingRegisters(1, 0, 10);
        
        if (resp.success) {
            // Procesar datos
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void setup() {
    ModbusMgr.begin(Serial1, 4, 5, 9600);
    xTaskCreate(modbusTask, "Modbus", 4096, NULL, 1, NULL);
}
```

## 📊 Estadísticas

```cpp
ModbusMgr.printStats();
```

Salida:
```
╔════════════════════════════════════════╗
║   Modbus Manager - Estadísticas        ║
╚════════════════════════════════════════╝
  Total peticiones: 150
  Peticiones exitosas: 145
  Peticiones fallidas: 5
  Timeouts: 2
  Errores CRC: 1
  Excepciones: 2
  Tasa de éxito: 96.7%
════════════════════════════════════════
```

## 🛡️ Thread Safety

Todas las operaciones están protegidas con mutex FreeRTOS. Safe para múltiples tasks.

## 📄 Licencia

MIT License

---

**Desarrollado para Nehuentue Suit Sensor v2.0**
