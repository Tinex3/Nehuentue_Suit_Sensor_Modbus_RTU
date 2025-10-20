/**
 * @file example_flash_storage.cpp
 * @brief Ejemplo de uso de FlashStorageManager
 * 
 * Este ejemplo muestra cómo:
 * 1. Inicializar el storage
 * 2. Guardar/cargar estructuras con CRC
 * 3. Guardar/cargar strings y primitivos
 * 4. Verificar existencia de keys
 * 5. Ver estadísticas
 */

#include <Arduino.h>
#include <FlashStorageManager.h>

// Estructura de ejemplo para configuración completa
struct DeviceConfig {
    // WiFi
    char ssid[32];
    char password[64];
    char deviceId[32];
    
    // MQTT
    char mqttServer[64];
    uint16_t mqttPort;
    char mqttUser[32];
    char mqttPassword[64];
    
    // Sensor
    uint8_t modbusAddress;
    uint8_t modbusFunction;
    uint16_t registerStart;
    uint16_t registerCount;
    float multiplier;
    float offset;
    
    // Estado
    bool enabled;
};

void printConfig(const DeviceConfig& config) {
    Serial.println("\n═══ Configuración Actual ═══");
    Serial.printf("WiFi SSID: %s\n", config.ssid);
    Serial.printf("Device ID: %s\n", config.deviceId);
    Serial.printf("MQTT Server: %s:%d\n", config.mqttServer, config.mqttPort);
    Serial.printf("MQTT User: %s\n", config.mqttUser);
    Serial.printf("Modbus Addr: %d, Func: 0x%02X\n", config.modbusAddress, config.modbusFunction);
    Serial.printf("Registers: Start=%d, Count=%d\n", config.registerStart, config.registerCount);
    Serial.printf("Formula: Value = (Raw × %.2f) + %.2f\n", config.multiplier, config.offset);
    Serial.printf("Enabled: %s\n", config.enabled ? "Yes" : "No");
    Serial.println("═══════════════════════════\n");
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║  FlashStorageManager - Ejemplo         ║");
    Serial.println("╚════════════════════════════════════════╝\n");
    
    // ========================================================================
    // 1. INICIALIZAR
    // ========================================================================
    Serial.println("1️⃣  Inicializando FlashStorage...");
    FlashStorageStatus status = FlashStorage.begin("nehuentue");
    
    if (status != FLASH_STORAGE_OK) {
        Serial.println("❌ Error al inicializar FlashStorage");
        return;
    }
    Serial.println("✅ FlashStorage inicializado\n");
    
    // ========================================================================
    // 2. VERIFICAR SI EXISTE CONFIGURACIÓN
    // ========================================================================
    Serial.println("2️⃣  Verificando configuración existente...");
    
    DeviceConfig config;
    bool configExists = FlashStorage.exists("device_config");
    
    if (configExists) {
        Serial.println("📦 Configuración encontrada en flash");
        
        // Cargar con verificación CRC
        status = FlashStorage.load("device_config", config);
        
        if (status == FLASH_STORAGE_OK) {
            Serial.println("✅ Configuración cargada y verificada (CRC OK)");
            printConfig(config);
        } else if (status == FLASH_STORAGE_ERROR_CRC_MISMATCH) {
            Serial.println("⚠️  Datos corruptos (CRC mismatch), usando defaults");
            configExists = false;
        } else if (status == FLASH_STORAGE_ERROR_VERSION_MISMATCH) {
            Serial.println("⚠️  Versión incompatible, usando defaults");
            configExists = false;
        } else {
            Serial.printf("❌ Error al cargar: %d\n", status);
            configExists = false;
        }
    }
    
    // ========================================================================
    // 3. CREAR CONFIGURACIÓN POR DEFECTO SI NO EXISTE
    // ========================================================================
    if (!configExists) {
        Serial.println("\n3️⃣  Creando configuración por defecto...");
        
        // WiFi
        strcpy(config.ssid, "Amanda 2.4G");
        strcpy(config.password, "");  // No guardar contraseña en ejemplo
        strcpy(config.deviceId, "modbus-01");
        
        // MQTT
        strcpy(config.mqttServer, "192.168.1.25");
        config.mqttPort = 1883;
        strcpy(config.mqttUser, "mqttuser");
        strcpy(config.mqttPassword, "");  // No guardar contraseña en ejemplo
        
        // Sensor
        config.modbusAddress = 1;
        config.modbusFunction = 0x03;
        config.registerStart = 0;
        config.registerCount = 10;
        config.multiplier = 1.0f;
        config.offset = 0.0f;
        config.enabled = true;
        
        printConfig(config);
        
        // Guardar con CRC
        status = FlashStorage.save("device_config", config);
        
        if (status == FLASH_STORAGE_OK) {
            Serial.println("✅ Configuración guardada en flash\n");
        } else {
            Serial.printf("❌ Error al guardar: %d\n\n", status);
        }
    }
    
    // ========================================================================
    // 4. EJEMPLO: GUARDAR/CARGAR STRINGS Y PRIMITIVOS
    // ========================================================================
    Serial.println("4️⃣  Ejemplo de strings y primitivos...");
    
    // Strings
    FlashStorage.saveString("test_string", "Hello FlashStorage!");
    String loadedString = FlashStorage.loadString("test_string", "default");
    Serial.printf("String: %s\n", loadedString.c_str());
    
    // Enteros
    FlashStorage.saveInt("boot_count", 42);
    int bootCount = FlashStorage.loadInt("boot_count", 0);
    Serial.printf("Boot count: %d\n", bootCount);
    
    // Booleanos
    FlashStorage.saveBool("first_run", false);
    bool firstRun = FlashStorage.loadBool("first_run", true);
    Serial.printf("First run: %s\n", firstRun ? "Yes" : "No");
    
    // Flotantes
    FlashStorage.saveFloat("temperature", 25.5f);
    float temperature = FlashStorage.loadFloat("temperature", 0.0f);
    Serial.printf("Temperature: %.1f°C\n\n", temperature);
    
    // ========================================================================
    // 5. ESTADÍSTICAS
    // ========================================================================
    Serial.println("5️⃣  Estadísticas de uso:");
    FlashStorage.printStats();
    
    // ========================================================================
    // 6. VERIFICAR ESPACIO DISPONIBLE
    // ========================================================================
    Serial.println("6️⃣  Información de espacio:");
    size_t freeEntries = FlashStorage.getFreeEntries();
    Serial.printf("Entradas libres en NVS: %zu\n\n", freeEntries);
    
    // ========================================================================
    // 7. EJEMPLO: ACTUALIZAR CONFIGURACIÓN
    // ========================================================================
    Serial.println("7️⃣  Ejemplo de actualización:");
    Serial.println("Incrementando contador de arranques...");
    
    int newBootCount = bootCount + 1;
    FlashStorage.saveInt("boot_count", newBootCount);
    Serial.printf("Nuevo boot count: %d\n\n", newBootCount);
    
    // ========================================================================
    // 8. CLEANUP (OPCIONAL - DESCOMENTA PARA PROBAR)
    // ========================================================================
    // Serial.println("8️⃣  Limpieza (DESHABILITADO):");
    // Serial.println("Descomenta las siguientes líneas para limpiar el storage:");
    // Serial.println("  FlashStorage.remove(\"test_string\");");
    // Serial.println("  FlashStorage.clear();  // ⚠️  Elimina TODO\n");
    
    // ========================================================================
    // FIN
    // ========================================================================
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║  Ejemplo completado exitosamente       ║");
    Serial.println("╚════════════════════════════════════════╝\n");
    
    Serial.println("💡 Reinicia el ESP32 para verificar que");
    Serial.println("   la configuración persiste entre reinicios.");
}

void loop() {
    // Nada que hacer
    delay(1000);
}
