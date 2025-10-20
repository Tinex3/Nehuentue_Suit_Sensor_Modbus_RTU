/**
 * @file ModbusManager.cpp
 * @brief Implementación del ModbusManager
 * @version 1.0.0
 * @date 2025-10-19
 */

#include "ModbusManager.h"

// Instancia global
ModbusManager ModbusMgr;

// ============================================================================
// CONSTRUCTOR Y DESTRUCTOR
// ============================================================================

ModbusManager::ModbusManager() {
    initialized = false;
    mutex = NULL;
    taskHandle = NULL;
    requestQueue = NULL;
    responseCallback = nullptr;
    
    memset(&config, 0, sizeof(ModbusConfig));
    memset(&stats, 0, sizeof(ModbusStats));
}

ModbusManager::~ModbusManager() {
    end();
}

// ============================================================================
// INICIALIZACIÓN
// ============================================================================

bool ModbusManager::begin(HardwareSerial& serial, int rxPin, int txPin, unsigned long baudrate) {
    if (initialized) {
        Serial.println("[MODBUS MGR] Ya inicializado");
        return true;
    }
    
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║   Modbus Manager v1.0                  ║");
    Serial.println("╚════════════════════════════════════════╝");
    
    // Crear mutex
    mutex = xSemaphoreCreateMutex();
    if (mutex == NULL) {
        Serial.println("[MODBUS MGR] ERROR: No se pudo crear mutex");
        return false;
    }
    
    // Crear cola de peticiones
    requestQueue = xQueueCreate(MODBUS_MGR_QUEUE_SIZE, sizeof(ModbusRequest));
    if (requestQueue == NULL) {
        Serial.println("[MODBUS MGR] ERROR: No se pudo crear cola");
        vSemaphoreDelete(mutex);
        return false;
    }
    
    // Configurar puerto serial
    config.serial = &serial;
    config.rxPin = rxPin;
    config.txPin = txPin;
    config.baudrate = baudrate;
    config.timeout = MODBUS_MGR_TIMEOUT_MS;
    
    serial.begin(baudrate, SERIAL_8N1, rxPin, txPin);
    
    initialized = true;
    
    Serial.printf("  Puerto: %s\n", "Serial1");
    Serial.printf("  RX Pin: GPIO %d\n", rxPin);
    Serial.printf("  TX Pin: GPIO %d\n", txPin);
    Serial.printf("  Baudrate: %lu bps\n", baudrate);
    Serial.printf("  Timeout: %lu ms\n", config.timeout);
    Serial.println("════════════════════════════════════════\n");
    
    return true;
}

void ModbusManager::end() {
    if (initialized) {
        initialized = false;
        
        if (taskHandle != NULL) {
            vTaskDelete(taskHandle);
            taskHandle = NULL;
        }
        
        Serial.println("[MODBUS MGR] Finalizado");
    }
    
    if (mutex != NULL) {
        vSemaphoreDelete(mutex);
        mutex = NULL;
    }
    
    if (requestQueue != NULL) {
        vQueueDelete(requestQueue);
        requestQueue = NULL;
    }
}

// ============================================================================
// FUNCIONES MODBUS
// ============================================================================

ModbusResponse ModbusManager::readHoldingRegisters(uint8_t slaveId, uint16_t startAddress, uint16_t quantity) {
    if (!initialized) {
        ModbusResponse empty = {0};
        empty.success = false;
        return empty;
    }
    
    uint8_t request[6];
    request[0] = slaveId;
    request[1] = MODBUS_READ_HOLDING_REGISTERS;
    request[2] = (uint8_t)(startAddress >> 8);
    request[3] = (uint8_t)(startAddress & 0xFF);
    request[4] = (uint8_t)(quantity >> 8);
    request[5] = (uint8_t)(quantity & 0xFF);
    
    return sendRequest(request, 6);
}

ModbusResponse ModbusManager::readInputRegisters(uint8_t slaveId, uint16_t startAddress, uint16_t quantity) {
    if (!initialized) {
        ModbusResponse empty = {0};
        empty.success = false;
        return empty;
    }
    
    uint8_t request[6];
    request[0] = slaveId;
    request[1] = MODBUS_READ_INPUT_REGISTERS;
    request[2] = (uint8_t)(startAddress >> 8);
    request[3] = (uint8_t)(startAddress & 0xFF);
    request[4] = (uint8_t)(quantity >> 8);
    request[5] = (uint8_t)(quantity & 0xFF);
    
    return sendRequest(request, 6);
}

ModbusResponse ModbusManager::readCoils(uint8_t slaveId, uint16_t startAddress, uint16_t quantity) {
    if (!initialized) {
        ModbusResponse empty = {0};
        empty.success = false;
        return empty;
    }
    
    uint8_t request[6];
    request[0] = slaveId;
    request[1] = MODBUS_READ_COILS;
    request[2] = (uint8_t)(startAddress >> 8);
    request[3] = (uint8_t)(startAddress & 0xFF);
    request[4] = (uint8_t)(quantity >> 8);
    request[5] = (uint8_t)(quantity & 0xFF);
    
    return sendRequest(request, 6);
}

ModbusResponse ModbusManager::writeSingleRegister(uint8_t slaveId, uint16_t address, uint16_t value) {
    if (!initialized) {
        ModbusResponse empty = {0};
        empty.success = false;
        return empty;
    }
    
    uint8_t request[6];
    request[0] = slaveId;
    request[1] = MODBUS_WRITE_SINGLE_REGISTER;
    request[2] = (uint8_t)(address >> 8);
    request[3] = (uint8_t)(address & 0xFF);
    request[4] = (uint8_t)(value >> 8);
    request[5] = (uint8_t)(value & 0xFF);
    
    return sendRequest(request, 6);
}

ModbusResponse ModbusManager::writeMultipleRegisters(uint8_t slaveId, uint16_t startAddress, uint16_t quantity, uint16_t* values) {
    if (!initialized) {
        ModbusResponse empty = {0};
        empty.success = false;
        return empty;
    }
    
    uint8_t byteCount = quantity * 2;
    uint8_t request[7 + byteCount];
    
    request[0] = slaveId;
    request[1] = MODBUS_WRITE_MULTIPLE_REGISTERS;
    request[2] = (uint8_t)(startAddress >> 8);
    request[3] = (uint8_t)(startAddress & 0xFF);
    request[4] = (uint8_t)(quantity >> 8);
    request[5] = (uint8_t)(quantity & 0xFF);
    request[6] = byteCount;
    
    for (uint16_t i = 0; i < quantity; i++) {
        request[7 + i*2] = (uint8_t)(values[i] >> 8);
        request[7 + i*2 + 1] = (uint8_t)(values[i] & 0xFF);
    }
    
    return sendRequest(request, 7 + byteCount);
}

// ============================================================================
// UTILIDADES
// ============================================================================

uint16_t ModbusManager::calculateCRC(const uint8_t* buf, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buf[pos];
        for (int i = 0; i < 8; i++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc;
}

bool ModbusManager::verifyCRC(const uint8_t* buf, size_t len) {
    if (len < 3) return false;
    uint16_t recvCrc = (uint16_t)buf[len - 2] | ((uint16_t)buf[len - 1] << 8);
    uint16_t calcCrc = calculateCRC(buf, len - 2);
    return (recvCrc == calcCrc);
}

uint16_t ModbusManager::extractRegisters(const ModbusResponse& response, uint16_t* registers, size_t maxRegisters) {
    if (!response.success || response.length < 5) {
        return 0;
    }
    
    uint8_t byteCount = response.data[2];
    uint16_t registerCount = byteCount / 2;
    
    if (registerCount > maxRegisters) {
        registerCount = maxRegisters;
    }
    
    for (uint16_t i = 0; i < registerCount; i++) {
        registers[i] = ((uint16_t)response.data[3 + i*2] << 8) | response.data[4 + i*2];
    }
    
    return registerCount;
}

const char* ModbusManager::getExceptionDescription(uint8_t exceptionCode) {
    switch (exceptionCode) {
        case 0x01: return "Función ilegal";
        case 0x02: return "Dirección de datos ilegal";
        case 0x03: return "Valor de datos ilegal";
        case 0x04: return "Fallo del dispositivo esclavo";
        case 0x05: return "Reconocer";
        case 0x06: return "Dispositivo esclavo ocupado";
        case 0x08: return "Error de paridad de memoria";
        case 0x0A: return "Gateway path unavailable";
        case 0x0B: return "Gateway target device failed to respond";
        default: return "Excepción desconocida";
    }
}

// ============================================================================
// CALLBACKS
// ============================================================================

void ModbusManager::onResponse(ModbusResponseCallback callback) {
    lock();
    responseCallback = callback;
    unlock();
}

// ============================================================================
// CONFIGURACIÓN
// ============================================================================

void ModbusManager::setTimeout(uint32_t timeout) {
    lock();
    config.timeout = timeout;
    unlock();
}

// ============================================================================
// ESTADÍSTICAS
// ============================================================================

void ModbusManager::resetStats() {
    lock();
    memset(&stats, 0, sizeof(ModbusStats));
    unlock();
}

void ModbusManager::printStats() {
    lock();
    
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║   Modbus Manager - Estadísticas        ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.printf("  Total peticiones: %lu\n", stats.totalRequests);
    Serial.printf("  Peticiones exitosas: %lu\n", stats.successfulRequests);
    Serial.printf("  Peticiones fallidas: %lu\n", stats.failedRequests);
    Serial.printf("  Timeouts: %lu\n", stats.timeouts);
    Serial.printf("  Errores CRC: %lu\n", stats.crcErrors);
    Serial.printf("  Excepciones: %lu\n", stats.exceptions);
    Serial.printf("  Última petición: %lu ms\n", stats.lastRequestTime);
    Serial.printf("  Última respuesta: %lu ms\n", stats.lastResponseTime);
    
    if (stats.totalRequests > 0) {
        float successRate = (float)stats.successfulRequests / stats.totalRequests * 100.0f;
        Serial.printf("  Tasa de éxito: %.1f%%\n", successRate);
    }
    
    Serial.println("════════════════════════════════════════\n");
    
    unlock();
}

void ModbusManager::printInfo() {
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║   Modbus Manager - Información         ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.printf("  RX Pin: GPIO %d\n", config.rxPin);
    Serial.printf("  TX Pin: GPIO %d\n", config.txPin);
    Serial.printf("  Baudrate: %lu bps\n", config.baudrate);
    Serial.printf("  Timeout: %lu ms\n", config.timeout);
    Serial.printf("  Inicializado: %s\n", initialized ? "Sí" : "No");
    Serial.println("════════════════════════════════════════\n");
}

// ============================================================================
// MÉTODOS PRIVADOS
// ============================================================================

void ModbusManager::lock() {
    if (mutex != NULL) {
        xSemaphoreTake(mutex, portMAX_DELAY);
    }
}

void ModbusManager::unlock() {
    if (mutex != NULL) {
        xSemaphoreGive(mutex);
    }
}

ModbusResponse ModbusManager::sendRequest(uint8_t* request, size_t requestLength) {
    ModbusResponse response;
    memset(&response, 0, sizeof(ModbusResponse));
    response.success = false;
    
    if (!initialized || config.serial == NULL) {
        return response;
    }
    
    lock();
    
    stats.totalRequests++;
    stats.lastRequestTime = millis();
    
    // Limpiar buffer de entrada
    while (config.serial->available()) {
        config.serial->read();
    }
    
    // Añadir CRC
    uint8_t fullRequest[requestLength + 2];
    memcpy(fullRequest, request, requestLength);
    uint16_t crc = calculateCRC(request, requestLength);
    fullRequest[requestLength] = (uint8_t)(crc & 0xFF);
    fullRequest[requestLength + 1] = (uint8_t)(crc >> 8);
    
    // Enviar petición
    config.serial->write(fullRequest, requestLength + 2);
    config.serial->flush();
    
    // Esperar respuesta
    unsigned long startTime = millis();
    size_t bytesRead = 0;
    
    while (millis() - startTime < config.timeout && bytesRead < MODBUS_MGR_MAX_RESPONSE_SIZE) {
        if (config.serial->available()) {
            response.data[bytesRead++] = config.serial->read();
            startTime = millis();  // Reset timeout
        }
        
        // Verificar si respuesta completa
        if (bytesRead >= 5) {
            uint8_t functionCode = response.data[1];
            
            // Excepción
            if (functionCode & 0x80) {
                if (bytesRead >= 5) break;
            }
            // Función 0x03 o 0x04
            else if (functionCode == 0x03 || functionCode == 0x04) {
                if (bytesRead >= 3) {
                    uint8_t byteCount = response.data[2];
                    if (bytesRead >= (5 + byteCount)) break;
                }
            }
            // Función 0x01 o 0x02
            else if (functionCode == 0x01 || functionCode == 0x02) {
                if (bytesRead >= 3) {
                    uint8_t byteCount = response.data[2];
                    if (bytesRead >= (5 + byteCount)) break;
                }
            }
            // Función 0x06 o 0x10
            else if (functionCode == 0x06 || functionCode == 0x10) {
                if (bytesRead >= 8) break;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    response.length = bytesRead;
    stats.lastResponseTime = millis();
    
    // Timeout
    if (bytesRead == 0) {
        stats.timeouts++;
        stats.failedRequests++;
        unlock();
        return response;
    }
    
    // Verificar CRC
    if (!verifyCRC(response.data, bytesRead)) {
        stats.crcErrors++;
        stats.failedRequests++;
        unlock();
        return response;
    }
    
    // Verificar excepción
    if ((response.data[1] & 0x80) != 0) {
        response.exceptionCode = response.data[2];
        response.slaveId = response.data[0];
        response.functionCode = response.data[1] & 0x7F;
        stats.exceptions++;
        stats.failedRequests++;
        unlock();
        return response;
    }
    
    // Respuesta exitosa
    response.success = true;
    response.slaveId = response.data[0];
    response.functionCode = response.data[1];
    response.timestamp = millis();
    stats.successfulRequests++;
    
    unlock();
    
    // Callback
    if (responseCallback != nullptr) {
        responseCallback(response);
    }
    
    return response;
}

void ModbusManager::modbusTask(void* parameter) {
    ModbusManager* mgr = (ModbusManager*)parameter;
    
    while (true) {
        ModbusRequest req;
        if (xQueueReceive(mgr->requestQueue, &req, pdMS_TO_TICKS(100)) == pdTRUE) {
            mgr->processRequest(req);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void ModbusManager::processRequest(const ModbusRequest& req) {
    // Procesamiento de peticiones encoladas (uso futuro)
    // Por ahora las peticiones son síncronas
}
