/**
 * @file SystemManager.h
 * @brief Coordinador global del sistema Nehuentue Sensor
 * @version 1.0.0
 * @date 2025-10-19
 * 
 * SystemManager coordina todos los módulos del sistema:
 * - FlashStorageManager
 * - WiFiManager
 * - MQTTManager
 * - ModbusManager
 * - WebServerManager
 */

#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

#include <Arduino.h>

// ============================================================================
// ESTRUCTURAS GLOBALES
// ============================================================================

/**
 * @brief Estado global del sistema
 */
struct SystemStatus {
    bool wifiConnected;
    bool mqttConnected;
    bool modbusEnabled;
    bool webServerRunning;
    uint32_t uptime;
    uint32_t freeHeap;
    uint32_t minFreeHeap;
    float cpuFreqMHz;
    const char* firmwareVersion;
    const char* chipModel;
    uint8_t chipRevision;
};

/**
 * @brief Información de firmware
 */
struct FirmwareInfo {
    const char* version;
    const char* buildDate;
    const char* buildTime;
    const char* author;
    const char* project;
};

// ============================================================================
// CLASE SYSTEMMANAGER
// ============================================================================

class SystemManager {
public:
    // Constructor
    SystemManager();
    ~SystemManager();
    
    // ========================================================================
    // INICIALIZACIÓN
    // ========================================================================
    
    /**
     * @brief Inicializar sistema completo
     * @return true si inicialización exitosa
     */
    bool begin();
    
    /**
     * @brief Loop principal del sistema
     * Debe llamarse frecuentemente en loop()
     */
    void loop();
    
    // ========================================================================
    // ESTADO DEL SISTEMA
    // ========================================================================
    
    /**
     * @brief Obtener estado del sistema
     * @return Estructura con estado actual
     */
    SystemStatus getStatus();
    
    /**
     * @brief Obtener información de firmware
     * @return Estructura con info de firmware
     */
    FirmwareInfo getFirmwareInfo();
    
    /**
     * @brief Obtener uptime en milisegundos
     * @return Tiempo desde inicio
     */
    uint32_t getUptime() { return millis(); }
    
    /**
     * @brief Obtener memoria libre
     * @return Bytes de heap libre
     */
    uint32_t getFreeHeap() { return ESP.getFreeHeap(); }
    
    /**
     * @brief Obtener información detallada de RAM usando ESP-IDF
     */
    void getMemoryInfo(uint32_t& totalRam, uint32_t& freeRam, uint32_t& usedRam, 
                       uint32_t& minFreeRam, uint32_t& largestFreeBlock);
    
    /**
     * @brief Obtener información detallada de Flash usando ESP-IDF
     */
    void getFlashInfo(uint32_t& totalFlash, uint32_t& usedFlash, uint32_t& freeFlash,
                      uint32_t& appSize, uint32_t& otaSize);
    
    // ========================================================================
    // REINICIO Y RESET
    // ========================================================================
    
    /**
     * @brief Reiniciar ESP32
     * @param delayMs Delay antes de reiniciar (ms)
     */
    void restart(uint32_t delayMs = 1000);
    
    /**
     * @brief Reset configuración completa
     */
    void resetConfiguration();
    
    /**
     * @brief Reset a valores de fábrica
     */
    void factoryReset();
    
    // ========================================================================
    // UTILIDADES
    // ========================================================================
    
    /**
     * @brief Imprimir información del sistema
     */
    void printInfo();
    
    /**
     * @brief Imprimir estado de todos los módulos
     */
    void printStatus();
    
    /**
     * @brief Obtener razón del último boot
     * @return Descripción de la razón
     */
    const char* getBootReason();
    
    /**
     * @brief Obtener chip ID único
     * @return Chip ID como string
     */
    String getChipId();

private:
    bool initialized;
    uint32_t startTime;
    
    // Información de firmware
    static constexpr const char* FW_VERSION = "2.0.0";
    static constexpr const char* FW_BUILD_DATE = __DATE__;
    static constexpr const char* FW_BUILD_TIME = __TIME__;
    static constexpr const char* FW_AUTHOR = "Nehuentue";
    static constexpr const char* FW_PROJECT = "Suit Sensor Modbus RTU";
};

// Instancia global
extern SystemManager SysMgr;

#endif // SYSTEM_MANAGER_H
