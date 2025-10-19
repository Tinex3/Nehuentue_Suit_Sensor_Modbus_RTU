#ifndef EEPROM_MANAGER_H
#define EEPROM_MANAGER_H

#include <Arduino.h>
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// ============================================================================
// CONFIGURACIÓN EEPROM 24LCXX (Compatible con 24LC64, 24LC128, 24LC256, etc.)
// ============================================================================
#define EEPROM_I2C_ADDRESS 0x50     // Dirección I2C de la EEPROM (ajustable)

// Selecciona tu modelo de EEPROM:
// 24LC64:  8192 bytes (8 KB)
// 24LC128: 16384 bytes (16 KB)
// 24LC256: 32768 bytes (32 KB)
#define EEPROM_SIZE 16384           // 128 Kbit = 16 KBytes (24LC128)

#define EEPROM_PAGE_SIZE 32         // Tamaño de página para escritura (igual en todos)
#define EEPROM_WRITE_CYCLE_TIME_MS 5 // Tiempo de escritura típico

// ============================================================================
// CONFIGURACIÓN I2C ESP32
// ============================================================================
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000   // 100 kHz (configurable)
#define I2C_MASTER_TIMEOUT_MS 100
#define I2C_ACK_CHECK_EN true
#define I2C_ACK_CHECK_DIS false

// ============================================================================
// ENUMERACIONES Y ESTADOS
// ============================================================================

// Estados de retorno de operaciones
enum EEPROMStatus {
    EEPROM_OK = 0,
    EEPROM_ERROR_TIMEOUT,
    EEPROM_ERROR_ADDRESS_OUT_OF_RANGE,
    EEPROM_ERROR_WRITE_FAILED,
    EEPROM_ERROR_READ_FAILED,
    EEPROM_ERROR_CRC_FAILED,
    EEPROM_ERROR_NOT_INITIALIZED,
    EEPROM_ERROR_INVALID_SIZE,
    EEPROM_ERROR_NULL_POINTER
};

// ============================================================================
// CLASE EEPROM MANAGER (ULTRA GENÉRICA)
// ============================================================================
class EEPROMManager {
private:
    bool initialized;
    SemaphoreHandle_t i2cMutex;
    uint8_t deviceAddress;
    int sdaPin;
    int sclPin;
    uint32_t frequency;
    uint16_t eepromSize;  // Tamaño configurable de la EEPROM
    
    // Funciones privadas de bajo nivel
    esp_err_t writeRaw(uint16_t address, const uint8_t* data, size_t length);
    esp_err_t readRaw(uint16_t address, uint8_t* buffer, size_t length);
    bool verifyDevice();
    
public:
    // ========================================================================
    // CONSTRUCTOR Y DESTRUCTOR
    // ========================================================================
    EEPROMManager();
    ~EEPROMManager();
    
    // ========================================================================
    // INICIALIZACIÓN Y CONFIGURACIÓN
    // ========================================================================
    EEPROMStatus begin(int sdaPin, int sclPin, uint16_t size = EEPROM_SIZE, uint32_t frequency = I2C_MASTER_FREQ_HZ, uint8_t deviceAddr = EEPROM_I2C_ADDRESS);
    void end();
    bool isReady();
    void setDeviceAddress(uint8_t address);
    uint16_t getSize();  // Obtiene el tamaño configurado de la EEPROM
    
    // ========================================================================
    // OPERACIONES BÁSICAS (Byte/Bytes)
    // ========================================================================
    EEPROMStatus writeByte(uint16_t address, uint8_t data);
    EEPROMStatus writeBytes(uint16_t address, const uint8_t* data, size_t length);
    EEPROMStatus readByte(uint16_t address, uint8_t* data);
    EEPROMStatus readBytes(uint16_t address, uint8_t* buffer, size_t length);
    
    // ========================================================================
    // OPERACIONES GENÉRICAS CON TEMPLATES (Ultra flexible)
    // ========================================================================
    
    // Guarda cualquier estructura/tipo de dato
    template<typename T>
    EEPROMStatus save(uint16_t address, const T& data) {
        if (address + sizeof(T) > eepromSize) {
            return EEPROM_ERROR_ADDRESS_OUT_OF_RANGE;
        }
        return writeBytes(address, (const uint8_t*)&data, sizeof(T));
    }
    
    // Carga cualquier estructura/tipo de dato
    template<typename T>
    EEPROMStatus load(uint16_t address, T& data) {
        if (address + sizeof(T) > eepromSize) {
            return EEPROM_ERROR_ADDRESS_OUT_OF_RANGE;
        }
        return readBytes(address, (uint8_t*)&data, sizeof(T));
    }
    
    // Guarda con verificación CRC16
    template<typename T>
    EEPROMStatus saveWithCRC(uint16_t address, const T& data) {
        if (address + sizeof(T) + 2 > eepromSize) {
            return EEPROM_ERROR_ADDRESS_OUT_OF_RANGE;
        }
        
        // Guarda datos
        EEPROMStatus status = save(address, data);
        if (status != EEPROM_OK) return status;
        
        // Calcula y guarda CRC
        uint16_t crc = calculateCRC16((const uint8_t*)&data, sizeof(T));
        return save(address + sizeof(T), crc);
    }
    
    // Carga con verificación CRC16
    template<typename T>
    EEPROMStatus loadWithCRC(uint16_t address, T& data) {
        if (address + sizeof(T) + 2 > eepromSize) {
            return EEPROM_ERROR_ADDRESS_OUT_OF_RANGE;
        }
        
        // Carga datos
        EEPROMStatus status = load(address, data);
        if (status != EEPROM_OK) return status;
        
        // Carga y verifica CRC
        uint16_t crcStored, crcCalculated;
        status = load(address + sizeof(T), crcStored);
        if (status != EEPROM_OK) return status;
        
        crcCalculated = calculateCRC16((const uint8_t*)&data, sizeof(T));
        
        if (crcStored != crcCalculated) {
            Serial.printf("[EEPROM] CRC mismatch: stored=0x%04X, calc=0x%04X\n", crcStored, crcCalculated);
            return EEPROM_ERROR_CRC_FAILED;
        }
        
        return EEPROM_OK;
    }
    
    // ========================================================================
    // OPERACIONES CON STRINGS
    // ========================================================================
    EEPROMStatus saveString(uint16_t address, const String& str, uint16_t maxLength = 256);
    EEPROMStatus loadString(uint16_t address, String& str, uint16_t maxLength = 256);
    EEPROMStatus saveCString(uint16_t address, const char* str, uint16_t maxLength = 256);
    EEPROMStatus loadCString(uint16_t address, char* buffer, uint16_t maxLength = 256);
    
    // ========================================================================
    // OPERACIONES CON ARRAYS
    // ========================================================================
    template<typename T>
    EEPROMStatus saveArray(uint16_t address, const T* array, size_t count) {
        size_t totalSize = sizeof(T) * count;
        if (address + totalSize > eepromSize) {
            return EEPROM_ERROR_ADDRESS_OUT_OF_RANGE;
        }
        return writeBytes(address, (const uint8_t*)array, totalSize);
    }
    
    template<typename T>
    EEPROMStatus loadArray(uint16_t address, T* array, size_t count) {
        size_t totalSize = sizeof(T) * count;
        if (address + totalSize > eepromSize) {
            return EEPROM_ERROR_ADDRESS_OUT_OF_RANGE;
        }
        return readBytes(address, (uint8_t*)array, totalSize);
    }
    
    // ========================================================================
    // OPERACIONES DE UTILIDAD
    // ========================================================================
    EEPROMStatus clear(uint16_t startAddress, uint16_t length);
    EEPROMStatus clearAll();
    EEPROMStatus fill(uint16_t startAddress, uint16_t length, uint8_t value);
    
    // ========================================================================
    // INFORMACIÓN Y DIAGNÓSTICO
    // ========================================================================
    uint16_t getTotalSize() const { return eepromSize; }
    uint16_t getPageSize() const { return EEPROM_PAGE_SIZE; }
    uint8_t getDeviceAddress() const { return deviceAddress; }
    uint16_t getFreeSpace(uint16_t fromAddress = 0);
    void printStatus();
    void printMemoryMap(uint16_t startAddress = 0, uint16_t length = 256);
    void dumpMemory(uint16_t startAddress, uint16_t length);
    
    // ========================================================================
    // UTILIDADES CRC
    // ========================================================================
    uint16_t calculateCRC16(const uint8_t* data, size_t length);
    
    template<typename T>
    uint16_t calculateCRC16(const T& data) {
        return calculateCRC16((const uint8_t*)&data, sizeof(T));
    }
};

// ============================================================================
// INSTANCIA GLOBAL
// ============================================================================
extern EEPROMManager EEPROM24LC64;

#endif // EEPROM_MANAGER_H
