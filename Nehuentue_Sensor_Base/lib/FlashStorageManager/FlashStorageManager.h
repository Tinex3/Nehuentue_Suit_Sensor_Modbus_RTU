/**
 * @file FlashStorageManager.h
 * @brief Manager para almacenamiento persistente en flash (NVS/Preferences) con FreeRTOS
 * @version 1.0.0
 * @date 2025-10-19
 * 
 * @details
 * Librería thread-safe para guardar/cargar configuración en la flash interna del ESP32.
 * Usa el sistema NVS (Non-Volatile Storage) a través de la API Preferences de Arduino.
 * 
 * Características:
 * - Thread-safe con mutexes FreeRTOS
 * - Verificación CRC16 para integridad de datos
 * - Versionado de configuración
 * - Soporte para estructuras, strings y tipos primitivos
 * - API simple tipo template (genérico)
 * - Límites de escritura para proteger flash (10,000 ciclos típicos)
 * 
 * Uso:
 * @code
 * FlashStorageManager storage;
 * storage.begin("myapp");
 * 
 * MyConfig config;
 * if (storage.load("config", config)) {
 *     Serial.println("Configuración cargada");
 * } else {
 *     // Usar valores por defecto
 *     storage.save("config", config);
 * }
 * @endcode
 */

#ifndef FLASH_STORAGE_MANAGER_H
#define FLASH_STORAGE_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// ============================================================================
// CONSTANTES Y CONFIGURACIÓN
// ============================================================================

#define FLASH_STORAGE_VERSION 1
#define FLASH_STORAGE_MAX_KEY_LENGTH 15        // Límite de NVS
#define FLASH_STORAGE_MAX_STRING_LENGTH 512    // Máximo para strings
#define FLASH_STORAGE_MAX_BLOB_SIZE 4000       // Máximo para blobs (4KB)
#define FLASH_STORAGE_TIMEOUT_MS 1000          // Timeout para mutex

// ============================================================================
// ENUMERACIONES
// ============================================================================

/**
 * @brief Estados de retorno de operaciones
 */
enum FlashStorageStatus {
    FLASH_STORAGE_OK = 0,
    FLASH_STORAGE_ERROR_NOT_INITIALIZED,
    FLASH_STORAGE_ERROR_KEY_TOO_LONG,
    FLASH_STORAGE_ERROR_SIZE_TOO_LARGE,
    FLASH_STORAGE_ERROR_WRITE_FAILED,
    FLASH_STORAGE_ERROR_READ_FAILED,
    FLASH_STORAGE_ERROR_KEY_NOT_FOUND,
    FLASH_STORAGE_ERROR_CRC_MISMATCH,
    FLASH_STORAGE_ERROR_VERSION_MISMATCH,
    FLASH_STORAGE_ERROR_TIMEOUT,
    FLASH_STORAGE_ERROR_NULL_POINTER
};

// ============================================================================
// ESTRUCTURAS
// ============================================================================

/**
 * @brief Encabezado para datos guardados con CRC y versión
 */
struct FlashStorageHeader {
    uint16_t crc;           // CRC16 de los datos
    uint16_t version;       // Versión de la estructura
    uint32_t size;          // Tamaño de los datos
    uint32_t timestamp;     // Timestamp de guardado (epoch)
};

/**
 * @brief Estadísticas de uso del almacenamiento
 */
struct FlashStorageStats {
    uint32_t totalWrites;
    uint32_t totalReads;
    uint32_t crcErrors;
    uint32_t versionMismatches;
    unsigned long lastWriteTime;
    unsigned long lastReadTime;
};

// ============================================================================
// CLASE PRINCIPAL
// ============================================================================

/**
 * @brief Manager para almacenamiento persistente en flash NVS
 * 
 * Esta clase proporciona una API simple y thread-safe para guardar y cargar
 * configuración en la memoria flash interna del ESP32.
 */
class FlashStorageManager {
public:
    // ========================================================================
    // CONSTRUCTOR Y DESTRUCTOR
    // ========================================================================
    
    FlashStorageManager();
    ~FlashStorageManager();
    
    // ========================================================================
    // INICIALIZACIÓN
    // ========================================================================
    
    /**
     * @brief Inicializa el storage con un namespace
     * @param namespaceName Nombre del namespace NVS (max 15 chars)
     * @param readOnly true para modo solo lectura
     * @return FlashStorageStatus código de estado
     */
    FlashStorageStatus begin(const char* namespaceName, bool readOnly = false);
    
    /**
     * @brief Finaliza y libera recursos
     */
    void end();
    
    /**
     * @brief Verifica si está inicializado
     * @return true si está listo para usar
     */
    bool isReady() const { return initialized; }
    
    // ========================================================================
    // OPERACIONES GENÉRICAS (Templates)
    // ========================================================================
    
    /**
     * @brief Guarda una estructura/objeto en flash
     * @tparam T Tipo de dato a guardar
     * @param key Clave (max 15 chars)
     * @param data Referencia al dato
     * @param useHeader true para incluir CRC y versión
     * @return FlashStorageStatus código de estado
     */
    template<typename T>
    FlashStorageStatus save(const char* key, const T& data, bool useHeader = true) {
        if (!initialized) return FLASH_STORAGE_ERROR_NOT_INITIALIZED;
        if (strlen(key) > FLASH_STORAGE_MAX_KEY_LENGTH) return FLASH_STORAGE_ERROR_KEY_TOO_LONG;
        
        // Tomar mutex
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(FLASH_STORAGE_TIMEOUT_MS)) != pdTRUE) {
            return FLASH_STORAGE_ERROR_TIMEOUT;
        }
        
        FlashStorageStatus status = FLASH_STORAGE_OK;
        
        if (useHeader) {
            // Crear header con CRC
            FlashStorageHeader header;
            header.crc = calculateCRC16((const uint8_t*)&data, sizeof(T));
            header.version = FLASH_STORAGE_VERSION;
            header.size = sizeof(T);
            header.timestamp = millis() / 1000;
            
            // Combinar header + data en un buffer
            size_t totalSize = sizeof(FlashStorageHeader) + sizeof(T);
            uint8_t* buffer = new uint8_t[totalSize];
            memcpy(buffer, &header, sizeof(FlashStorageHeader));
            memcpy(buffer + sizeof(FlashStorageHeader), &data, sizeof(T));
            
            // Guardar blob completo
            size_t written = preferences.putBytes(key, buffer, totalSize);
            delete[] buffer;
            
            if (written != totalSize) {
                status = FLASH_STORAGE_ERROR_WRITE_FAILED;
            } else {
                stats.totalWrites++;
                stats.lastWriteTime = millis();
            }
        } else {
            // Guardar directamente sin header
            size_t written = preferences.putBytes(key, (const void*)&data, sizeof(T));
            if (written != sizeof(T)) {
                status = FLASH_STORAGE_ERROR_WRITE_FAILED;
            } else {
                stats.totalWrites++;
                stats.lastWriteTime = millis();
            }
        }
        
        xSemaphoreGive(mutex);
        return status;
    }
    
    /**
     * @brief Carga una estructura/objeto desde flash
     * @tparam T Tipo de dato a cargar
     * @param key Clave (max 15 chars)
     * @param data Referencia donde guardar el dato
     * @param useHeader true si se guardó con header
     * @return FlashStorageStatus código de estado
     */
    template<typename T>
    FlashStorageStatus load(const char* key, T& data, bool useHeader = true) {
        if (!initialized) return FLASH_STORAGE_ERROR_NOT_INITIALIZED;
        if (strlen(key) > FLASH_STORAGE_MAX_KEY_LENGTH) return FLASH_STORAGE_ERROR_KEY_TOO_LONG;
        
        // Tomar mutex
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(FLASH_STORAGE_TIMEOUT_MS)) != pdTRUE) {
            return FLASH_STORAGE_ERROR_TIMEOUT;
        }
        
        FlashStorageStatus status = FLASH_STORAGE_OK;
        
        if (useHeader) {
            // Calcular tamaño total
            size_t totalSize = sizeof(FlashStorageHeader) + sizeof(T);
            
            // Verificar que la key existe
            if (!preferences.isKey(key)) {
                xSemaphoreGive(mutex);
                return FLASH_STORAGE_ERROR_KEY_NOT_FOUND;
            }
            
            // Leer blob completo
            uint8_t* buffer = new uint8_t[totalSize];
            size_t read = preferences.getBytes(key, buffer, totalSize);
            
            if (read != totalSize) {
                delete[] buffer;
                xSemaphoreGive(mutex);
                return FLASH_STORAGE_ERROR_READ_FAILED;
            }
            
            // Extraer header
            FlashStorageHeader header;
            memcpy(&header, buffer, sizeof(FlashStorageHeader));
            
            // Verificar versión
            if (header.version != FLASH_STORAGE_VERSION) {
                delete[] buffer;
                xSemaphoreGive(mutex);
                stats.versionMismatches++;
                return FLASH_STORAGE_ERROR_VERSION_MISMATCH;
            }
            
            // Extraer datos
            memcpy(&data, buffer + sizeof(FlashStorageHeader), sizeof(T));
            
            // Verificar CRC
            uint16_t calculatedCRC = calculateCRC16((const uint8_t*)&data, sizeof(T));
            if (calculatedCRC != header.crc) {
                delete[] buffer;
                xSemaphoreGive(mutex);
                stats.crcErrors++;
                return FLASH_STORAGE_ERROR_CRC_MISMATCH;
            }
            
            delete[] buffer;
            stats.totalReads++;
            stats.lastReadTime = millis();
            
        } else {
            // Leer directamente sin header
            if (!preferences.isKey(key)) {
                xSemaphoreGive(mutex);
                return FLASH_STORAGE_ERROR_KEY_NOT_FOUND;
            }
            
            size_t read = preferences.getBytes(key, (void*)&data, sizeof(T));
            if (read != sizeof(T)) {
                status = FLASH_STORAGE_ERROR_READ_FAILED;
            } else {
                stats.totalReads++;
                stats.lastReadTime = millis();
            }
        }
        
        xSemaphoreGive(mutex);
        return status;
    }
    
    // ========================================================================
    // OPERACIONES CON STRINGS
    // ========================================================================
    
    FlashStorageStatus saveString(const char* key, const String& value);
    FlashStorageStatus loadString(const char* key, String& value);
    String loadString(const char* key, const String& defaultValue = "");
    
    // ========================================================================
    // OPERACIONES CON TIPOS PRIMITIVOS
    // ========================================================================
    
    FlashStorageStatus saveInt(const char* key, int32_t value);
    FlashStorageStatus loadInt(const char* key, int32_t& value);
    int32_t loadInt(const char* key, int32_t defaultValue = 0);
    
    FlashStorageStatus saveUInt(const char* key, uint32_t value);
    FlashStorageStatus loadUInt(const char* key, uint32_t& value);
    uint32_t loadUInt(const char* key, uint32_t defaultValue = 0);
    
    FlashStorageStatus saveBool(const char* key, bool value);
    FlashStorageStatus loadBool(const char* key, bool& value);
    bool loadBool(const char* key, bool defaultValue = false);
    
    FlashStorageStatus saveFloat(const char* key, float value);
    FlashStorageStatus loadFloat(const char* key, float& value);
    float loadFloat(const char* key, float defaultValue = 0.0f);
    
    // ========================================================================
    // UTILIDADES
    // ========================================================================
    
    bool exists(const char* key);
    FlashStorageStatus remove(const char* key);
    FlashStorageStatus clear();
    size_t getFreeEntries();
    
    // ========================================================================
    // ESTADÍSTICAS Y DIAGNÓSTICO
    // ========================================================================
    
    FlashStorageStats getStats() const { return stats; }
    void resetStats();
    void printStats();
    void printAllKeys();
    
    // ========================================================================
    // CRC Y VALIDACIÓN
    // ========================================================================
    
    uint16_t calculateCRC16(const uint8_t* data, size_t length);
    
    template<typename T>
    uint16_t calculateCRC16(const T& data) {
        return calculateCRC16((const uint8_t*)&data, sizeof(T));
    }

private:
    Preferences preferences;
    SemaphoreHandle_t mutex;
    bool initialized;
    bool readOnly;
    char namespaceName[16];
    FlashStorageStats stats;
};

// ============================================================================
// INSTANCIA GLOBAL (opcional)
// ============================================================================
extern FlashStorageManager FlashStorage;

#endif // FLASH_STORAGE_MANAGER_H
