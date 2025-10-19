#include "eeprom_manager.h"

// Instancia global
EEPROMManager EEPROM24LC64;

// ============================================================================
// IMPLEMENTACIÓN DE LA CLASE EEPROMManager (ULTRA GENÉRICA)
// ============================================================================

EEPROMManager::EEPROMManager() {
    initialized = false;
    i2cMutex = NULL;
    deviceAddress = EEPROM_I2C_ADDRESS;
    sdaPin = -1;
    sclPin = -1;
    frequency = I2C_MASTER_FREQ_HZ;
    eepromSize = EEPROM_SIZE;  // Valor por defecto (24LC128)
}

EEPROMManager::~EEPROMManager() {
    end();
}

// Inicializa el driver I2C y la EEPROM
EEPROMStatus EEPROMManager::begin(int sda, int scl, uint16_t size, uint32_t freq, uint8_t devAddr) {
    sdaPin = sda;
    sclPin = scl;
    eepromSize = size;        // Configura el tamaño
    frequency = freq;
    deviceAddress = devAddr;
    
    // Crea mutex para operaciones I2C thread-safe
    if (i2cMutex == NULL) {
        i2cMutex = xSemaphoreCreateMutex();
        if (i2cMutex == NULL) {
            Serial.println("[EEPROM] ERROR: No se pudo crear mutex I2C");
            return EEPROM_ERROR_NOT_INITIALIZED;
        }
    }
    
    // Configuración I2C
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = (gpio_num_t)sda;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = (gpio_num_t)scl;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = freq;
    conf.clk_flags = 0;
    
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        Serial.printf("[EEPROM] ERROR: Configuración I2C falló (0x%x)\n", err);
        return EEPROM_ERROR_NOT_INITIALIZED;
    }
    
    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        Serial.printf("[EEPROM] ERROR: Instalación driver I2C falló (0x%x)\n", err);
        return EEPROM_ERROR_NOT_INITIALIZED;
    }
    
    // Verifica que la EEPROM esté presente
    vTaskDelay(pdMS_TO_TICKS(10)); // Pequeño delay para estabilización
    
    if (!verifyDevice()) {
        Serial.println("[EEPROM] ADVERTENCIA: EEPROM no detectada en bus I2C");
        Serial.println("[EEPROM] Continuando de todos modos (puede estar desconectada)");
    }
    
    initialized = true;
    
    // Determina el modelo según el tamaño
    const char* modelo = "24LCXX";
    if (eepromSize == 4096) modelo = "24LC32 (4KB)";
    else if (eepromSize == 8192) modelo = "24LC64 (8KB)";
    else if (eepromSize == 16384) modelo = "24LC128 (16KB)";
    else if (eepromSize == 32768) modelo = "24LC256 (32KB)";
    else if (eepromSize == 65536) modelo = "24LC512 (64KB)";
    
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║   EEPROM Manager v2.0 - Ultra Generic ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.printf("  Modelo: %s\n", modelo);
    Serial.printf("  Dirección I2C: 0x%02X\n", deviceAddress);
    Serial.printf("  SDA: GPIO %d\n", sda);
    Serial.printf("  SCL: GPIO %d\n", scl);
    Serial.printf("  Frecuencia: %lu Hz\n", freq);
    Serial.printf("  Tamaño: %d bytes\n", eepromSize);
    Serial.printf("  Página: %d bytes\n", EEPROM_PAGE_SIZE);
    Serial.printf("  Thread-safe: ✓\n");
    Serial.printf("  CRC16: ✓\n");
    Serial.println("════════════════════════════════════════\n");
    
    return EEPROM_OK;
}

// Finaliza y libera recursos
void EEPROMManager::end() {
    if (initialized) {
        i2c_driver_delete(I2C_MASTER_NUM);
        initialized = false;
        Serial.println("[EEPROM] Driver I2C finalizado");
    }
    
    if (i2cMutex != NULL) {
        vSemaphoreDelete(i2cMutex);
        i2cMutex = NULL;
    }
}

// Establece dirección del dispositivo I2C (para múltiples EEPROMs)
void EEPROMManager::setDeviceAddress(uint8_t address) {
    deviceAddress = address;
    Serial.printf("[EEPROM] Dirección del dispositivo cambiada a 0x%02X\n", address);
}

// Obtiene el tamaño configurado de la EEPROM
uint16_t EEPROMManager::getSize() {
    return eepromSize;
}

// Verifica que el dispositivo esté presente en el bus I2C
bool EEPROMManager::verifyDevice() {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (deviceAddress << 1) | I2C_MASTER_WRITE, I2C_ACK_CHECK_EN);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    
    return (ret == ESP_OK);
}

// Verifica si la EEPROM está lista
bool EEPROMManager::isReady() {
    if (!initialized) return false;
    return verifyDevice();
}

// Escritura raw (bajo nivel) - Thread-safe
esp_err_t EEPROMManager::writeRaw(uint16_t address, const uint8_t* data, size_t length) {
    if (!initialized) return ESP_ERR_INVALID_STATE;
    if (address + length > eepromSize) return ESP_ERR_INVALID_ARG;
    if (data == NULL) return ESP_ERR_INVALID_ARG;
    
    // Toma mutex
    if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS * 2)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    esp_err_t result = ESP_OK;
    size_t written = 0;
    
    // Escribe respetando límites de página
    while (written < length) {
        uint16_t currentAddress = address + written;
        size_t pageRemaining = EEPROM_PAGE_SIZE - (currentAddress % EEPROM_PAGE_SIZE);
        size_t toWrite = (pageRemaining < (length - written)) ? pageRemaining : (length - written);
        
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (deviceAddress << 1) | I2C_MASTER_WRITE, I2C_ACK_CHECK_EN);
        i2c_master_write_byte(cmd, (uint8_t)(currentAddress >> 8), I2C_ACK_CHECK_EN);
        i2c_master_write_byte(cmd, (uint8_t)(currentAddress & 0xFF), I2C_ACK_CHECK_EN);
        
        for (size_t i = 0; i < toWrite; i++) {
            i2c_master_write_byte(cmd, data[written + i], I2C_ACK_CHECK_EN);
        }
        
        i2c_master_stop(cmd);
        
        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
        i2c_cmd_link_delete(cmd);
        
        if (ret != ESP_OK) {
            result = ret;
            break;
        }
        
        written += toWrite;
        vTaskDelay(pdMS_TO_TICKS(EEPROM_WRITE_CYCLE_TIME_MS));
    }
    
    // Libera mutex
    xSemaphoreGive(i2cMutex);
    
    return result;
}

// Lectura raw (bajo nivel) - Thread-safe
esp_err_t EEPROMManager::readRaw(uint16_t address, uint8_t* buffer, size_t length) {
    if (!initialized) return ESP_ERR_INVALID_STATE;
    if (address + length > eepromSize) return ESP_ERR_INVALID_ARG;
    if (buffer == NULL) return ESP_ERR_INVALID_ARG;
    
    // Toma mutex
    if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS * 2)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    // Escribe dirección
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (deviceAddress << 1) | I2C_MASTER_WRITE, I2C_ACK_CHECK_EN);
    i2c_master_write_byte(cmd, (uint8_t)(address >> 8), I2C_ACK_CHECK_EN);
    i2c_master_write_byte(cmd, (uint8_t)(address & 0xFF), I2C_ACK_CHECK_EN);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        xSemaphoreGive(i2cMutex);
        return ret;
    }
    
    // Lee datos
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (deviceAddress << 1) | I2C_MASTER_READ, I2C_ACK_CHECK_EN);
    
    if (length > 1) {
        i2c_master_read(cmd, buffer, length - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, &buffer[length - 1], I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    
    // Libera mutex
    xSemaphoreGive(i2cMutex);
    
    return ret;
}

// Calcula CRC16 (Modbus/CCITT)
uint16_t EEPROMManager::calculateCRC16(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc;
}

// ============================================================================
// OPERACIONES BÁSICAS
// ============================================================================

EEPROMStatus EEPROMManager::writeByte(uint16_t address, uint8_t data) {
    esp_err_t err = writeRaw(address, &data, 1);
    return (err == ESP_OK) ? EEPROM_OK : EEPROM_ERROR_WRITE_FAILED;
}

EEPROMStatus EEPROMManager::writeBytes(uint16_t address, const uint8_t* data, size_t length) {
    if (data == NULL) return EEPROM_ERROR_NULL_POINTER;
    esp_err_t err = writeRaw(address, data, length);
    return (err == ESP_OK) ? EEPROM_OK : EEPROM_ERROR_WRITE_FAILED;
}

EEPROMStatus EEPROMManager::readByte(uint16_t address, uint8_t* data) {
    if (data == NULL) return EEPROM_ERROR_NULL_POINTER;
    esp_err_t err = readRaw(address, data, 1);
    return (err == ESP_OK) ? EEPROM_OK : EEPROM_ERROR_READ_FAILED;
}

EEPROMStatus EEPROMManager::readBytes(uint16_t address, uint8_t* buffer, size_t length) {
    if (buffer == NULL) return EEPROM_ERROR_NULL_POINTER;
    esp_err_t err = readRaw(address, buffer, length);
    return (err == ESP_OK) ? EEPROM_OK : EEPROM_ERROR_READ_FAILED;
}

// ============================================================================
// OPERACIONES CON STRINGS
// ============================================================================

EEPROMStatus EEPROMManager::saveString(uint16_t address, const String& str, uint16_t maxLength) {
    uint16_t length = str.length();
    if (length > maxLength) length = maxLength;
    
    // Guarda longitud primero (2 bytes)
    EEPROMStatus status = save(address, length);
    if (status != EEPROM_OK) return status;
    
    // Guarda string
    return writeBytes(address + 2, (const uint8_t*)str.c_str(), length);
}

EEPROMStatus EEPROMManager::loadString(uint16_t address, String& str, uint16_t maxLength) {
    // Lee longitud
    uint16_t length;
    EEPROMStatus status = load(address, length);
    if (status != EEPROM_OK) return status;
    
    if (length > maxLength) return EEPROM_ERROR_INVALID_SIZE;
    
    // Lee string
    char buffer[length + 1];
    status = readBytes(address + 2, (uint8_t*)buffer, length);
    if (status != EEPROM_OK) return status;
    
    buffer[length] = '\0';
    str = String(buffer);
    return EEPROM_OK;
}

EEPROMStatus EEPROMManager::saveCString(uint16_t address, const char* str, uint16_t maxLength) {
    if (str == NULL) return EEPROM_ERROR_NULL_POINTER;
    
    uint16_t length = strlen(str);
    if (length > maxLength - 1) length = maxLength - 1;
    
    // Guarda longitud
    EEPROMStatus status = save(address, length);
    if (status != EEPROM_OK) return status;
    
    // Guarda string + null terminator
    status = writeBytes(address + 2, (const uint8_t*)str, length);
    if (status != EEPROM_OK) return status;
    
    uint8_t nullTerm = 0;
    return writeByte(address + 2 + length, nullTerm);
}

EEPROMStatus EEPROMManager::loadCString(uint16_t address, char* buffer, uint16_t maxLength) {
    if (buffer == NULL) return EEPROM_ERROR_NULL_POINTER;
    
    // Lee longitud
    uint16_t length;
    EEPROMStatus status = load(address, length);
    if (status != EEPROM_OK) return status;
    
    if (length >= maxLength) return EEPROM_ERROR_INVALID_SIZE;
    
    // Lee string
    status = readBytes(address + 2, (uint8_t*)buffer, length);
    if (status != EEPROM_OK) return status;
    
    buffer[length] = '\0';
    return EEPROM_OK;
}

// ============================================================================
// OPERACIONES DE UTILIDAD
// ============================================================================

EEPROMStatus EEPROMManager::clear(uint16_t startAddress, uint16_t length) {
    if (startAddress + length > eepromSize) {
        return EEPROM_ERROR_ADDRESS_OUT_OF_RANGE;
    }
    
    return fill(startAddress, length, 0xFF);
}

EEPROMStatus EEPROMManager::clearAll() {
    Serial.println("[EEPROM] Borrando toda la EEPROM...");
    EEPROMStatus status = clear(0, eepromSize);
    if (status == EEPROM_OK) {
        Serial.println("[EEPROM] ✓ EEPROM borrada completamente");
    }
    return status;
}

EEPROMStatus EEPROMManager::fill(uint16_t startAddress, uint16_t length, uint8_t value) {
    if (startAddress + length > eepromSize) {
        return EEPROM_ERROR_ADDRESS_OUT_OF_RANGE;
    }
    
    uint8_t fillBuffer[EEPROM_PAGE_SIZE];
    memset(fillBuffer, value, EEPROM_PAGE_SIZE);
    
    uint16_t remaining = length;
    uint16_t address = startAddress;
    
    while (remaining > 0) {
        uint16_t toWrite = (remaining > EEPROM_PAGE_SIZE) ? EEPROM_PAGE_SIZE : remaining;
        
        esp_err_t err = writeRaw(address, fillBuffer, toWrite);
        if (err != ESP_OK) {
            return EEPROM_ERROR_WRITE_FAILED;
        }
        
        address += toWrite;
        remaining -= toWrite;
    }
    
    return EEPROM_OK;
}

// ============================================================================
// INFORMACIÓN Y DIAGNÓSTICO
// ============================================================================

uint16_t EEPROMManager::getFreeSpace(uint16_t fromAddress) {
    if (fromAddress >= eepromSize) return 0;
    return eepromSize - fromAddress;
}

void EEPROMManager::printStatus() {
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║     EEPROM Manager - Estado            ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.printf("  Inicializada: %s\n", initialized ? "✓ Sí" : "✗ No");
    Serial.printf("  Dispositivo detectado: %s\n", isReady() ? "✓ Sí" : "✗ No");
    Serial.printf("  Dirección I2C: 0x%02X\n", deviceAddress);
    Serial.printf("  Tamaño total: %d bytes\n", eepromSize);
    Serial.printf("  Tamaño de página: %d bytes\n", EEPROM_PAGE_SIZE);
    Serial.printf("  Pines I2C: SDA=%d, SCL=%d\n", sdaPin, sclPin);
    Serial.printf("  Frecuencia: %lu Hz\n", frequency);
    Serial.println("════════════════════════════════════════\n");
}

void EEPROMManager::printMemoryMap(uint16_t startAddress, uint16_t length) {
    Serial.printf("\n[EEPROM] Mapa de memoria (0x%04X - 0x%04X):\n", startAddress, startAddress + length - 1);
    Serial.println("────────────────────────────────────────");
    Serial.printf("Dirección   | Datos\n");
    Serial.println("────────────────────────────────────────");
    
    for (uint16_t i = 0; i < length; i += 16) {
        Serial.printf("0x%04X: ", startAddress + i);
        
        for (uint16_t j = 0; j < 16 && (i + j) < length; j++) {
            uint8_t data;
            readByte(startAddress + i + j, &data);
            Serial.printf("%02X ", data);
        }
        Serial.println();
    }
    Serial.println("────────────────────────────────────────\n");
}

void EEPROMManager::dumpMemory(uint16_t startAddress, uint16_t length) {
    Serial.printf("\n[EEPROM] Volcado de memoria (0x%04X, %d bytes):\n", startAddress, length);
    
    uint8_t buffer[length];
    if (readBytes(startAddress, buffer, length) == EEPROM_OK) {
        for (uint16_t i = 0; i < length; i++) {
            if (i % 16 == 0) {
                if (i > 0) Serial.println();
                Serial.printf("0x%04X: ", startAddress + i);
            }
            Serial.printf("%02X ", buffer[i]);
        }
        Serial.println("\n");
    } else {
        Serial.println("[EEPROM] ERROR: No se pudo leer memoria\n");
    }
}
