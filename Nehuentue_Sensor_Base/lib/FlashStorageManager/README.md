# 📦 FlashStorageManager

**Librería thread-safe para almacenamiento persistente en ESP32 usando NVS (Preferences)**

Versión: 1.0.0  
Autor: Nehuentue Project  
Fecha: 19 de octubre de 2025

---

## 📋 Características

- ✅ **Thread-safe** con mutexes FreeRTOS
- ✅ **CRC16** para verificación de integridad
- ✅ **Versionado** de estructuras
- ✅ **Templates genéricos** para cualquier tipo de dato
- ✅ **API simple** para strings y primitivos
- ✅ **Estadísticas** de uso
- ✅ **Sin EEPROM externa** (usa flash interna del ESP32)

---

## 🚀 Instalación

Esta librería ya está incluida en el proyecto. PlatformIO la detecta automáticamente desde `lib/FlashStorageManager/`.

---

## 📖 Uso Básico

### 1. Inicialización

```cpp
#include <FlashStorageManager.h>

void setup() {
    // Usar instancia global
    FlashStorage.begin("myapp");  // Namespace (max 15 chars)
    
    // O crear instancia propia
    FlashStorageManager storage;
    storage.begin("config");
}
```

### 2. Guardar y Cargar Strings

```cpp
// Guardar
FlashStorage.saveString("ssid", "Mi_Red_WiFi");
FlashStorage.saveString("password", "mi_password_123");

// Cargar
String ssid = FlashStorage.loadString("ssid", "default_ssid");
String password = FlashStorage.loadString("password");
```

### 3. Guardar y Cargar Primitivos

```cpp
// Enteros
FlashStorage.saveInt("counter", 42);
int counter = FlashStorage.loadInt("counter", 0);

// Booleanos
FlashStorage.saveBool("enabled", true);
bool enabled = FlashStorage.loadBool("enabled", false);

// Flotantes
FlashStorage.saveFloat("temperature", 25.5f);
float temp = FlashStorage.loadFloat("temperature", 0.0f);
```

### 4. Guardar y Cargar Estructuras (con CRC y versión)

```cpp
struct MyConfig {
    char ssid[32];
    char password[64];
    uint8_t channel;
    bool enabled;
};

MyConfig config;
strcpy(config.ssid, "Mi_Red");
strcpy(config.password, "password123");
config.channel = 6;
config.enabled = true;

// Guardar con CRC y versión
FlashStorage.save("config", config);  // useHeader=true (default)

// Cargar con verificación
MyConfig loadedConfig;
FlashStorageStatus status = FlashStorage.load("config", loadedConfig);

if (status == FLASH_STORAGE_OK) {
    Serial.println("✓ Configuración cargada y verificada");
} else if (status == FLASH_STORAGE_ERROR_KEY_NOT_FOUND) {
    Serial.println("Primera vez, usando defaults");
} else if (status == FLASH_STORAGE_ERROR_CRC_MISMATCH) {
    Serial.println("¡Datos corruptos! Usando defaults");
}
```

### 5. Verificar Existencia

```cpp
if (FlashStorage.exists("config")) {
    Serial.println("Configuración encontrada");
} else {
    Serial.println("No hay configuración guardada");
}
```

### 6. Eliminar y Limpiar

```cpp
// Eliminar una key
FlashStorage.remove("old_config");

// Limpiar todo el namespace
FlashStorage.clear();
```

---

## 🎯 Ejemplo Completo: Configuración WiFi

```cpp
#include <Arduino.h>
#include <FlashStorageManager.h>

struct WiFiConfig {
    char ssid[32];
    char password[64];
    char deviceId[32];
    bool autoConnect;
};

void setup() {
    Serial.begin(115200);
    
    // Inicializar storage
    FlashStorage.begin("nehuentue");
    
    WiFiConfig config;
    
    // Intentar cargar configuración guardada
    if (FlashStorage.load("wifi", config) == FLASH_STORAGE_OK) {
        Serial.println("✓ Configuración cargada desde flash");
        Serial.printf("  SSID: %s\n", config.ssid);
        Serial.printf("  Device ID: %s\n", config.deviceId);
        Serial.printf("  Auto Connect: %s\n", config.autoConnect ? "Yes" : "No");
    } else {
        // Primera vez o datos corruptos - usar defaults
        Serial.println("⚠ No hay configuración, usando defaults");
        strcpy(config.ssid, "Default_SSID");
        strcpy(config.password, "password123");
        strcpy(config.deviceId, "modbus-01");
        config.autoConnect = true;
        
        // Guardar defaults
        FlashStorage.save("wifi", config);
        Serial.println("✓ Configuración default guardada");
    }
    
    // Usar configuración...
    WiFi.begin(config.ssid, config.password);
    
    // Mostrar estadísticas
    FlashStorage.printStats();
}

void loop() {
    delay(1000);
}
```

---

## 📊 Estadísticas

```cpp
// Obtener estadísticas
FlashStorageStats stats = FlashStorage.getStats();
Serial.printf("Escrituras: %lu\n", stats.totalWrites);
Serial.printf("Lecturas: %lu\n", stats.totalReads);
Serial.printf("Errores CRC: %lu\n", stats.crcErrors);

// Imprimir estadísticas formateadas
FlashStorage.printStats();

// Resetear estadísticas
FlashStorage.resetStats();
```

---

## 🔧 API Completa

### Inicialización
```cpp
FlashStorageStatus begin(const char* namespaceName, bool readOnly = false);
void end();
bool isReady() const;
```

### Estructuras (Templates)
```cpp
template<typename T>
FlashStorageStatus save(const char* key, const T& data, bool useHeader = true);

template<typename T>
FlashStorageStatus load(const char* key, T& data, bool useHeader = true);
```

### Strings
```cpp
FlashStorageStatus saveString(const char* key, const String& value);
FlashStorageStatus loadString(const char* key, String& value);
String loadString(const char* key, const String& defaultValue = "");
```

### Primitivos
```cpp
FlashStorageStatus saveInt(const char* key, int32_t value);
int32_t loadInt(const char* key, int32_t defaultValue = 0);

FlashStorageStatus saveUInt(const char* key, uint32_t value);
uint32_t loadUInt(const char* key, uint32_t defaultValue = 0);

FlashStorageStatus saveBool(const char* key, bool value);
bool loadBool(const char* key, bool defaultValue = false);

FlashStorageStatus saveFloat(const char* key, float value);
float loadFloat(const char* key, float defaultValue = 0.0f);
```

### Utilidades
```cpp
bool exists(const char* key);
FlashStorageStatus remove(const char* key);
FlashStorageStatus clear();
size_t getFreeEntries();
```

### Estadísticas
```cpp
FlashStorageStats getStats() const;
void resetStats();
void printStats();
```

### CRC
```cpp
uint16_t calculateCRC16(const uint8_t* data, size_t length);

template<typename T>
uint16_t calculateCRC16(const T& data);
```

---

## ⚠️ Límites y Consideraciones

### Límites de NVS (ESP32)
- **Namespace:** máximo 15 caracteres
- **Key:** máximo 15 caracteres
- **String:** hasta 512 bytes
- **Blob:** hasta 4000 bytes (4KB)
- **Entradas:** depende de la partición NVS (típicamente ~500 keys)

### Ciclos de Escritura
- **Flash NOR:** ~100,000 ciclos típicos
- **Wear leveling:** NVS lo maneja automáticamente
- **Recomendación:** No escribir continuamente, solo al cambiar configuración

### Thread-Safety
- ✅ Todos los métodos son thread-safe
- ✅ Usa mutexes FreeRTOS internamente
- ✅ Timeout configurable (1000ms default)

---

## 🐛 Códigos de Error

```cpp
enum FlashStorageStatus {
    FLASH_STORAGE_OK = 0,                       // ✓ Éxito
    FLASH_STORAGE_ERROR_NOT_INITIALIZED,        // No se llamó begin()
    FLASH_STORAGE_ERROR_KEY_TOO_LONG,           // Key > 15 chars
    FLASH_STORAGE_ERROR_SIZE_TOO_LARGE,         // Dato muy grande
    FLASH_STORAGE_ERROR_WRITE_FAILED,           // Escritura falló
    FLASH_STORAGE_ERROR_READ_FAILED,            // Lectura falló
    FLASH_STORAGE_ERROR_KEY_NOT_FOUND,          // Key no existe
    FLASH_STORAGE_ERROR_CRC_MISMATCH,           // Datos corruptos
    FLASH_STORAGE_ERROR_VERSION_MISMATCH,       // Versión incompatible
    FLASH_STORAGE_ERROR_TIMEOUT,                // Timeout del mutex
    FLASH_STORAGE_ERROR_NULL_POINTER            // Puntero nulo
};
```

---

## 🧪 Testing

```cpp
void testFlashStorage() {
    FlashStorage.begin("test");
    
    // Test strings
    FlashStorage.saveString("test_key", "test_value");
    String value = FlashStorage.loadString("test_key");
    assert(value == "test_value");
    
    // Test estructuras
    struct TestData {
        int x;
        float y;
    } data = {42, 3.14f};
    
    FlashStorage.save("struct", data);
    
    TestData loaded;
    FlashStorageStatus status = FlashStorage.load("struct", loaded);
    assert(status == FLASH_STORAGE_OK);
    assert(loaded.x == 42);
    assert(loaded.y == 3.14f);
    
    // Cleanup
    FlashStorage.clear();
    FlashStorage.end();
}
```

---

## 📚 Ejemplos Adicionales

Ver carpeta `examples/` (próximamente)

---

## 🤝 Contribuciones

Esta librería es parte del proyecto Nehuentue Suit Sensor Modbus RTU.

---

## 📄 Licencia

(Especificar licencia)

---

## 🔗 Referencias

- [ESP32 NVS Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html)
- [Arduino Preferences Library](https://github.com/espressif/arduino-esp32/tree/master/libraries/Preferences)
- [FreeRTOS Semaphores](https://www.freertos.org/a00113.html)
