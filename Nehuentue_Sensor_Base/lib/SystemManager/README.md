# 🎛️ SystemManager - Coordinador Global del Sistema

**Versión:** 1.0.0  
**Firmware:** Nehuentue Suit Sensor v2.0.0

## 📋 Descripción

SystemManager es el coordinador central que gestiona todos los módulos del sistema y proporciona información unificada sobre el estado general del dispositivo.

## ✨ Características

- ✅ **Información del sistema**: Firmware, hardware, memoria
- ✅ **Estado global**: WiFi, MQTT, Modbus, WebServer
- ✅ **Uptime tracking**: Tiempo de funcionamiento
- ✅ **Reset y reinicio**: Control de reinicios y factory reset
- ✅ **Boot reason**: Razón del último arranque
- ✅ **Chip ID único**: Identificador del dispositivo

## 🚀 Uso Rápido

```cpp
#include <SystemManager.h>

void setup() {
    Serial.begin(115200);
    
    // Inicializar System Manager
    SysMgr.begin();
    
    // Imprimir información
    SysMgr.printInfo();
    SysMgr.printStatus();
}

void loop() {
    SysMgr.loop();
    delay(100);
}
```

## 📚 API

### Inicialización

```cpp
bool begin();           // Inicializar manager
void loop();            // Loop principal (monitoreo)
```

### Estado del Sistema

```cpp
SystemStatus getStatus();          // Obtener estado completo
FirmwareInfo getFirmwareInfo();    // Info de firmware
uint32_t getUptime();              // Uptime en ms
uint32_t getFreeHeap();            // Memoria libre
const char* getBootReason();       // Razón del boot
String getChipId();                // ID único del chip
```

### Control

```cpp
void restart(uint32_t delayMs = 1000);   // Reiniciar ESP32
void resetConfiguration();               // Reset configuración
void factoryReset();                     // Factory reset completo
```

### Utilidades

```cpp
void printInfo();      // Imprimir info del sistema
void printStatus();    // Imprimir estado actual
```

## 📝 Ejemplos

### Ejemplo 1: Información del Sistema

```cpp
void showSystemInfo() {
    auto info = SysMgr.getFirmwareInfo();
    
    Serial.printf("Firmware: %s\n", info.version);
    Serial.printf("Compilado: %s %s\n", info.buildDate, info.buildTime);
    Serial.printf("Chip ID: %s\n", SysMgr.getChipId().c_str());
    Serial.printf("Boot: %s\n", SysMgr.getBootReason());
}
```

### Ejemplo 2: Estado Global

```cpp
void checkSystemStatus() {
    SystemStatus status = SysMgr.getStatus();
    
    if (status.wifiConnected) {
        Serial.println("WiFi: OK");
    }
    
    if (status.mqttConnected) {
        Serial.println("MQTT: OK");
    }
    
    Serial.printf("Heap: %lu bytes\n", status.freeHeap);
    Serial.printf("Uptime: %lu s\n", status.uptime / 1000);
}
```

### Ejemplo 3: Reinicio Programado

```cpp
void scheduleRestart() {
    Serial.println("Reiniciando en 5 segundos...");
    SysMgr.restart(5000);
}
```

### Ejemplo 4: Factory Reset

```cpp
void handleFactoryReset() {
    Serial.println("Ejecutando factory reset...");
    SysMgr.factoryReset();  // Reset y reinicio automático
}
```

### Ejemplo 5: Monitoreo Continuo

```cpp
void loop() {
    static uint32_t lastPrint = 0;
    
    if (millis() - lastPrint > 60000) {  // Cada minuto
        SysMgr.printStatus();
        lastPrint = millis();
    }
    
    SysMgr.loop();
}
```

## 📊 Salida de Ejemplo

### printInfo()

```
╔════════════════════════════════════════╗
║   Información del Sistema              ║
╚════════════════════════════════════════╝
  Firmware: v2.0.0
  Compilado: Oct 19 2025 14:30:00
  Proyecto: Suit Sensor Modbus RTU
  Autor: Nehuentue
----------------------------------------
  Chip: ESP32-C3 rev3
  CPU: 160 MHz
  Chip ID: AA:BB:CC:DD:EE:FF
----------------------------------------
  Heap libre: 245632 bytes
  Heap mínimo: 220544 bytes
  Tamaño sketch: 892416 bytes
  Espacio libre: 2207232 bytes
----------------------------------------
  Razón boot: Power-on
════════════════════════════════════════
```

### printStatus()

```
╔════════════════════════════════════════╗
║   Estado del Sistema                   ║
╚════════════════════════════════════════╝
  WiFi: ✓ Conectado
  MQTT: ✓ Conectado
  Modbus: ✓ Habilitado
  WebServer: ✓ Ejecutando
----------------------------------------
  Uptime: 02h 15m 43s
  Heap libre: 230144 bytes
  CPU: 160 MHz
════════════════════════════════════════
```

## 🔧 Integración

SystemManager está diseñado para coordinarse con todos los demás managers:

```cpp
#include <SystemManager.h>
#include <WiFiManager.h>
#include <MQTTManager.h>
#include <ModbusManager.h>
#include <FlashStorageManager.h>

void setup() {
    // Inicializar sistema
    SysMgr.begin();
    
    // Inicializar módulos
    FlashStorageMgr.begin("nehuentue");
    WiFiMgr.begin("Nehuentue-Sensor");
    MqttMgr.begin("broker.mqtt.com", 1883);
    ModbusMgr.begin(Serial1, 4, 5, 9600);
    
    // Mostrar estado
    SysMgr.printStatus();
}
```

## 📄 Licencia

MIT License

---

**Desarrollado para Nehuentue Suit Sensor v2.0**
