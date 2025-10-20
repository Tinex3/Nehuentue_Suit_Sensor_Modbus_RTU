# 🔌 Integración de FlashStorageManager en el Proyecto

**Fecha:** 19 de octubre de 2025  
**Estado:** ✅ Librería implementada y compilada  
**Próximo paso:** Integrar en main.cpp y tasks.cpp

---

## ✅ Lo que ya está listo

### **Librería FlashStorageManager**
- ✅ `lib/FlashStorageManager/FlashStorageManager.h` (API completa)
- ✅ `lib/FlashStorageManager/FlashStorageManager.cpp` (implementación)
- ✅ `lib/FlashStorageManager/README.md` (documentación)
- ✅ `lib/FlashStorageManager/examples/example_flash_storage.cpp` (ejemplo)
- ✅ **Compilación exitosa** sin errores

---

## 🎯 Plan de Integración

### **Objetivo**
Que la configuración de **WiFi, MQTT y Sensor** persista entre reinicios del ESP32.

### **Archivos a Modificar**
1. `src/main.cpp` - Cargar configuración al inicio
2. `src/tasks.cpp` - Inicializar defaults y cargar desde flash
3. `src/web_server.cpp` - Guardar al recibir POST en `/api/config` y `/api/sensors`

---

## 📝 Código de Integración

### **1. En `src/main.cpp`**

Añadir al inicio:
```cpp
#include <FlashStorageManager.h>

// Al inicio de setup()
void setup() {
    Serial.begin(115200);
    Serial.println("Iniciando Nehuentue Sensor Base...");
    
    // Inicializar FlashStorage ANTES que todo
    FlashStorage.begin("nehuentue");
    
    // Resto del código...
    modbusRTUInit();
    initTasks();  // <- Aquí se cargará la config desde flash
    // ...
}
```

---

### **2. En `src/tasks.cpp`**

Modificar `initDefaultConfig()` para cargar desde flash:

```cpp
#include <FlashStorageManager.h>

// Estructura para guardar en flash
struct StoredConfig {
    // WiFi
    char ssid[32];
    char password[64];
    char deviceId[32];
    
    // MQTT
    char mqttServer[64];
    uint16_t mqttPort;
    char mqttUser[32];
    char mqttPassword[64];
    unsigned long telemetryInterval;
    unsigned long statusInterval;
    
    // Sensor
    char sensorName[32];
    char sensorType[16];
    char sensorUnit[16];
    uint8_t modbusFunction;
    uint8_t modbusAddress;
    uint16_t registerStart;
    uint16_t registerCount;
    float multiplier;
    float offset;
    uint8_t decimals;
    bool enabled;
};

void initDefaultConfig() {
    Serial.println("[CONFIG] Inicializando configuración...");
    
    StoredConfig stored;
    
    // Intentar cargar desde flash
    if (FlashStorage.load("config", stored) == FLASH_STORAGE_OK) {
        Serial.println("[CONFIG] ✓ Configuración cargada desde flash");
        
        // WiFi
        strncpy(wifiConfig.ssid, stored.ssid, sizeof(wifiConfig.ssid));
        strncpy(wifiConfig.password, stored.password, sizeof(wifiConfig.password));
        strncpy(wifiConfig.deviceId, stored.deviceId, sizeof(wifiConfig.deviceId));
        strncpy(wifiConfig.mqttServer, stored.mqttServer, sizeof(wifiConfig.mqttServer));
        wifiConfig.mqttPort = stored.mqttPort;
        strncpy(wifiConfig.mqttUser, stored.mqttUser, sizeof(wifiConfig.mqttUser));
        strncpy(wifiConfig.mqttPassword, stored.mqttPassword, sizeof(wifiConfig.mqttPassword));
        
        // MQTT
        mqttConfig.telemetryInterval = stored.telemetryInterval;
        mqttConfig.statusInterval = stored.statusInterval;
        
        // Sensor
        strncpy(sensorConfig.name, stored.sensorName, sizeof(sensorConfig.name));
        strncpy(sensorConfig.type, stored.sensorType, sizeof(sensorConfig.type));
        strncpy(sensorConfig.unit, stored.sensorUnit, sizeof(sensorConfig.unit));
        sensorConfig.modbusFunction = stored.modbusFunction;
        sensorConfig.modbusAddress = stored.modbusAddress;
        sensorConfig.registerStart = stored.registerStart;
        sensorConfig.registerCount = stored.registerCount;
        sensorConfig.multiplier = stored.multiplier;
        sensorConfig.offset = stored.offset;
        sensorConfig.decimals = stored.decimals;
        sensorConfig.enabled = stored.enabled;
        
    } else {
        Serial.println("[CONFIG] ⚠ No hay configuración guardada, usando defaults");
        
        // WiFi (defaults hardcoded)
        strcpy(wifiConfig.ssid, "Amanda 2.4G");
        strcpy(wifiConfig.password, "");
        strcpy(wifiConfig.deviceId, "modbus-01");
        strcpy(wifiConfig.mqttServer, "192.168.1.25");
        wifiConfig.mqttPort = 1883;
        strcpy(wifiConfig.mqttUser, "mqttuser");
        strcpy(wifiConfig.mqttPassword, "");
        
        // MQTT
        mqttConfig.telemetryInterval = 60000;
        mqttConfig.statusInterval = 60000;
        
        // Sensor
        strcpy(sensorConfig.name, "modbus_sensor");
        strcpy(sensorConfig.type, "modbus_generic");
        strcpy(sensorConfig.unit, "");
        sensorConfig.modbusFunction = 0x03;
        sensorConfig.modbusAddress = 1;
        sensorConfig.registerStart = 0;
        sensorConfig.registerCount = 10;
        sensorConfig.multiplier = 1.0;
        sensorConfig.offset = 0.0;
        sensorConfig.decimals = 2;
        sensorConfig.enabled = true;
        
        // Guardar defaults en flash para la próxima vez
        saveConfigToFlash();
    }
    
    // Imprimir configuración cargada
    Serial.printf("[CONFIG] ✓ Configuración cargada:\n");
    Serial.printf("  SSID: %s\n", wifiConfig.ssid);
    Serial.printf("  Device ID: %s\n", wifiConfig.deviceId);
    Serial.printf("  MQTT Server: %s:%d\n", wifiConfig.mqttServer, wifiConfig.mqttPort);
    Serial.printf("  MQTT User: %s\n", wifiConfig.mqttUser);
    Serial.printf("  Sensor: %s (%s) - %d registros\n", 
                 sensorConfig.name, sensorConfig.type, sensorConfig.registerCount);
    Serial.printf("  Estado: %s\n\n", sensorConfig.enabled ? "habilitado" : "deshabilitado");
}

// Nueva función para guardar
void saveConfigToFlash() {
    StoredConfig stored;
    
    // Copiar desde structs globales
    strncpy(stored.ssid, wifiConfig.ssid, sizeof(stored.ssid));
    strncpy(stored.password, wifiConfig.password, sizeof(stored.password));
    strncpy(stored.deviceId, wifiConfig.deviceId, sizeof(stored.deviceId));
    strncpy(stored.mqttServer, wifiConfig.mqttServer, sizeof(stored.mqttServer));
    stored.mqttPort = wifiConfig.mqttPort;
    strncpy(stored.mqttUser, wifiConfig.mqttUser, sizeof(stored.mqttUser));
    strncpy(stored.mqttPassword, wifiConfig.mqttPassword, sizeof(stored.mqttPassword));
    
    stored.telemetryInterval = mqttConfig.telemetryInterval;
    stored.statusInterval = mqttConfig.statusInterval;
    
    strncpy(stored.sensorName, sensorConfig.name, sizeof(stored.sensorName));
    strncpy(stored.sensorType, sensorConfig.type, sizeof(stored.sensorType));
    strncpy(stored.sensorUnit, sensorConfig.unit, sizeof(stored.sensorUnit));
    stored.modbusFunction = sensorConfig.modbusFunction;
    stored.modbusAddress = sensorConfig.modbusAddress;
    stored.registerStart = sensorConfig.registerStart;
    stored.registerCount = sensorConfig.registerCount;
    stored.multiplier = sensorConfig.multiplier;
    stored.offset = sensorConfig.offset;
    stored.decimals = sensorConfig.decimals;
    stored.enabled = sensorConfig.enabled;
    
    // Guardar con CRC
    if (FlashStorage.save("config", stored) == FLASH_STORAGE_OK) {
        Serial.println("[CONFIG] ✓ Configuración guardada en flash");
    } else {
        Serial.println("[CONFIG] ✗ Error al guardar configuración");
    }
}
```

---

### **3. En `src/web_server.cpp`**

Añadir guardado automático al recibir configuración:

```cpp
#include <FlashStorageManager.h>

// Declarar función externa
extern void saveConfigToFlash();

// En handleConfigUpdate() después de actualizar wifiConfig:
void handleConfigUpdate(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    // ... código existente que actualiza wifiConfig ...
    
    // AÑADIR AL FINAL:
    Serial.println("[WEB API] Guardando configuración en flash...");
    saveConfigToFlash();
    
    request->send(200, "application/json", "{\"success\":true}");
}

// En handleSensorConfig() después de actualizar sensorConfig:
void handleSensorConfig(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    // ... código existente que actualiza sensorConfig ...
    
    // AÑADIR AL FINAL:
    Serial.println("[WEB API] Guardando configuración en flash...");
    saveConfigToFlash();
    
    request->send(200, "application/json", "{\"success\":true}");
}
```

---

## 🚀 Pasos para Implementar

### **Paso 1: Backup** ✅ (Recomendado)
```bash
cd ~/Documentos/Github/Personal/Nehuentue_Suit_Sensor_Modbus_RTU
git add .
git commit -m "Backup antes de integrar FlashStorageManager"
```

### **Paso 2: Modificar archivos**
- Copiar código de arriba a `src/main.cpp`
- Copiar código de arriba a `src/tasks.cpp`
- Copiar código de arriba a `src/web_server.cpp`

### **Paso 3: Compilar**
```bash
platformio run
```

### **Paso 4: Subir**
```bash
platformio run --target upload
```

### **Paso 5: Probar**
1. Conectar a `http://192.168.4.1`
2. Configurar WiFi y sensor
3. Reiniciar ESP32
4. Verificar que la configuración persiste

---

## 🧪 Testing

### **Test 1: Primera ejecución**
- ✅ Debe mostrar: `"No hay configuración guardada, usando defaults"`
- ✅ Debe guardar defaults en flash

### **Test 2: Segundo arranque**
- ✅ Debe mostrar: `"Configuración cargada desde flash"`
- ✅ Debe usar la configuración guardada

### **Test 3: Configuración desde web**
- ✅ Cambiar SSID, MQTT, sensor desde web UI
- ✅ Debe mostrar: `"Guardando configuración en flash..."`
- ✅ Reiniciar y verificar que persiste

### **Test 4: Corrupción de datos**
- Simular corrupción modificando manualmente flash
- ✅ Debe detectar CRC mismatch
- ✅ Debe usar defaults

---

## 📊 Ventajas de Esta Integración

1. ✅ **Persistencia:** Configuración sobrevive a reinicios
2. ✅ **Thread-safe:** Múltiples tareas pueden acceder sin problemas
3. ✅ **Integridad:** CRC16 detecta datos corruptos
4. ✅ **Simplicidad:** API simple, solo 3 líneas de código
5. ✅ **Sin hardware extra:** Usa flash interna del ESP32
6. ✅ **Wear leveling:** NVS lo maneja automáticamente
7. ✅ **Estadísticas:** Puedes ver cuántas escrituras/lecturas

---

## ⚠️ Consideraciones

### **Límite de Escrituras**
- Flash NOR: ~100,000 ciclos
- **Solución:** Solo guardar al cambiar configuración (no cada segundo)

### **Tamaño de StoredConfig**
- Actual: ~350 bytes
- Límite NVS: 4000 bytes por blob
- ✅ Tenemos espacio de sobra

### **Namespace**
- Usamos `"nehuentue"` (10 chars < 15 max)
- Si necesitas múltiples namespaces, puedes crear instancias:
  ```cpp
  FlashStorageManager storage1, storage2;
  storage1.begin("nehuentue");
  storage2.begin("calibration");
  ```

---

## 🔄 Próximos Pasos (Arquitectura Modular)

Después de probar FlashStorageManager, continuaremos con:

1. ✅ **FlashStorageManager** ← HECHO
2. ⏳ **WiFiManager** ← Siguiente
3. ⏳ **MQTTManager**
4. ⏳ **ModbusManager**
5. ⏳ **WebServerManager**
6. ⏳ **SystemManager** (coordinador)

---

## 📚 Referencias

- Ver `lib/FlashStorageManager/README.md` para API completa
- Ver `lib/FlashStorageManager/examples/example_flash_storage.cpp` para ejemplo standalone

---

**¿Listo para integrar?** Avísame y modifico los archivos para ti, o hazlo manualmente siguiendo esta guía.
