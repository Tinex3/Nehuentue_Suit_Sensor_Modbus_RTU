#include "modbus_rtu.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// Mutex para proteger acceso al puerto serial
static SemaphoreHandle_t serialMutex = NULL;

// Calcula CRC16 Modbus
uint16_t modbusCalculateCRC(const uint8_t *buf, size_t len) {
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

// Verifica CRC de un mensaje recibido
bool modbusVerifyCRC(const uint8_t *buf, size_t len) {
    if (len < 3) return false;
    uint16_t recvCrc = (uint16_t)buf[len - 2] | ((uint16_t)buf[len - 1] << 8);
    uint16_t calcCrc = modbusCalculateCRC(buf, len - 2);
    return (recvCrc == calcCrc);
}

// Inicializa Modbus RTU Master
void modbusRTUInit(int rxPin, int txPin, unsigned long baudrate) {
    // Configura Serial1 con pines personalizados y baudrate
    MODBUS_SERIAL_PORT.begin(baudrate, SERIAL_8N1, rxPin, txPin);
    
    // Crea mutex para proteger acceso al puerto serial
    if (serialMutex == NULL) {
        serialMutex = xSemaphoreCreateMutex();
    }
    
    Serial.printf("Modbus RTU Master inicializado:\n");
    Serial.printf("  - RX Pin: GPIO %d\n", rxPin);
    Serial.printf("  - TX Pin: GPIO %d\n", txPin);
    Serial.printf("  - Baudrate: %lu bps\n", baudrate);
}

// Función genérica para enviar petición y recibir respuesta
ModbusResponse modbusSendRequest(uint8_t *request, size_t requestLength) {
    ModbusResponse response;
    response.length = 0;
    response.success = false;
    response.exceptionCode = 0;
    
    Serial.println("--- DEBUG: Iniciando petición Modbus ---");
    
    // Protege acceso al puerto serial
    if (serialMutex == NULL) {
        Serial.println("ERROR: Mutex no inicializado");
        return response;
    }
    
    Serial.println("DEBUG: Esperando acceso al puerto serial...");
    if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(MODBUS_TIMEOUT_MS)) != pdTRUE) {
        Serial.println("ERROR: Timeout esperando mutex del puerto serial");
        return response;
    }
    Serial.println("DEBUG: Acceso al puerto obtenido");
    
    // Limpia buffer de entrada
    int cleared = 0;
    while (MODBUS_SERIAL_PORT.available()) {
        MODBUS_SERIAL_PORT.read();
        cleared++;
    }
    if (cleared > 0) {
        Serial.printf("DEBUG: Buffer limpiado (%d bytes descartados)\n", cleared);
    }
    
    // Añade CRC al mensaje
    uint8_t fullRequest[requestLength + 2];
    memcpy(fullRequest, request, requestLength);
    uint16_t crc = modbusCalculateCRC(request, requestLength);
    fullRequest[requestLength] = (uint8_t)(crc & 0xFF);
    fullRequest[requestLength + 1] = (uint8_t)(crc >> 8);
    
    Serial.printf("DEBUG: CRC calculado: 0x%04X\n", crc);
    
    // Envía petición
    MODBUS_SERIAL_PORT.write(fullRequest, requestLength + 2);
    MODBUS_SERIAL_PORT.flush();
    
    Serial.printf(">>> Enviados %d bytes: ", requestLength + 2);
    for (size_t i = 0; i < requestLength + 2; i++) {
        Serial.printf("%02X ", fullRequest[i]);
    }
    Serial.println();
    
    // Espera respuesta con timeout
    Serial.println("DEBUG: Esperando respuesta...");
    unsigned long startTime = millis();
    size_t bytesRead = 0;
    bool firstByte = true;
    
    while (millis() - startTime < MODBUS_TIMEOUT_MS && bytesRead < MODBUS_MAX_RESPONSE_SIZE) {
        if (MODBUS_SERIAL_PORT.available()) {
            if (firstByte) {
                Serial.printf("DEBUG: Primer byte recibido después de %lu ms\n", millis() - startTime);
                firstByte = false;
            }
            response.data[bytesRead++] = MODBUS_SERIAL_PORT.read();
            // Reset timeout si estamos recibiendo datos
            startTime = millis();
        }
        vTaskDelay(pdMS_TO_TICKS(1));
        
        // Si ya tenemos al menos la cabecera, verificar si la respuesta está completa
        if (bytesRead >= 5) {
            uint8_t functionCode = response.data[1];
            
            // Verificar si es una excepción (bit más significativo en 1)
            if (functionCode & 0x80) {
                if (bytesRead >= 5) break; // Excepción completa: addr + func + exception + CRC(2)
            }
            // Para función 0x03 o 0x04: addr + func + bytecount + datos + CRC(2)
            else if (functionCode == 0x03 || functionCode == 0x04) {
                if (bytesRead >= 3) {
                    uint8_t byteCount = response.data[2];
                    if (bytesRead >= (5 + byteCount)) break; // Respuesta completa
                }
            }
            // Para función 0x01 o 0x02: addr + func + bytecount + datos + CRC(2)
            else if (functionCode == 0x01 || functionCode == 0x02) {
                if (bytesRead >= 3) {
                    uint8_t byteCount = response.data[2];
                    if (bytesRead >= (5 + byteCount)) break;
                }
            }
            // Para función 0x06: addr + func + addr(2) + value(2) + CRC(2) = 8 bytes
            else if (functionCode == 0x06) {
                if (bytesRead >= 8) break;
            }
            // Para función 0x10: addr + func + addr(2) + qty(2) + CRC(2) = 8 bytes
            else if (functionCode == 0x10) {
                if (bytesRead >= 8) break;
            }
        }
    }
    
    response.length = bytesRead;
    
    Serial.printf("DEBUG: Recepción completada. Total bytes: %d\n", bytesRead);
    
    // Libera mutex
    xSemaphoreGive(serialMutex);
    Serial.println("DEBUG: Mutex liberado");
    
    if (bytesRead == 0) {
        Serial.println("ERROR: Timeout - No se recibió respuesta");
        return response;
    }
    
    Serial.printf("<<< Recibidos %d bytes: ", bytesRead);
    for (size_t i = 0; i < bytesRead; i++) {
        Serial.printf("%02X ", response.data[i]);
    }
    Serial.println();
    
    // Verifica CRC
    Serial.println("DEBUG: Verificando CRC...");
    if (!modbusVerifyCRC(response.data, bytesRead)) {
        Serial.println("ERROR: CRC inválido en respuesta");
        uint16_t recvCrc = (uint16_t)response.data[bytesRead - 2] | ((uint16_t)response.data[bytesRead - 1] << 8);
        uint16_t calcCrc = modbusCalculateCRC(response.data, bytesRead - 2);
        Serial.printf("  CRC recibido: 0x%04X\n", recvCrc);
        Serial.printf("  CRC calculado: 0x%04X\n", calcCrc);
        return response;
    }
    Serial.println("DEBUG: CRC válido ✓");
    
    // Verifica si es una excepción
    if ((response.data[1] & 0x80) != 0) {
        response.exceptionCode = response.data[2];
        Serial.printf("EXCEPCIÓN Modbus: 0x%02X\n", response.exceptionCode);
        return response;
    }
    
    Serial.println("DEBUG: Respuesta exitosa ✓");
    response.success = true;
    return response;
}

// Función 0x03: Read Holding Registers
ModbusResponse modbusReadHoldingRegisters(uint8_t slaveId, uint16_t startAddress, uint16_t quantity) {
    uint8_t request[6];
    request[0] = slaveId;
    request[1] = 0x03;
    request[2] = (uint8_t)(startAddress >> 8);
    request[3] = (uint8_t)(startAddress & 0xFF);
    request[4] = (uint8_t)(quantity >> 8);
    request[5] = (uint8_t)(quantity & 0xFF);
    
    return modbusSendRequest(request, 6);
}

// Función 0x04: Read Input Registers
ModbusResponse modbusReadInputRegisters(uint8_t slaveId, uint16_t startAddress, uint16_t quantity) {
    uint8_t request[6];
    request[0] = slaveId;
    request[1] = 0x04;
    request[2] = (uint8_t)(startAddress >> 8);
    request[3] = (uint8_t)(startAddress & 0xFF);
    request[4] = (uint8_t)(quantity >> 8);
    request[5] = (uint8_t)(quantity & 0xFF);
    
    return modbusSendRequest(request, 6);
}

// Función 0x01: Read Coils
ModbusResponse modbusReadCoils(uint8_t slaveId, uint16_t startAddress, uint16_t quantity) {
    uint8_t request[6];
    request[0] = slaveId;
    request[1] = 0x01;
    request[2] = (uint8_t)(startAddress >> 8);
    request[3] = (uint8_t)(startAddress & 0xFF);
    request[4] = (uint8_t)(quantity >> 8);
    request[5] = (uint8_t)(quantity & 0xFF);
    
    return modbusSendRequest(request, 6);
}

// Función 0x06: Write Single Register
ModbusResponse modbusWriteSingleRegister(uint8_t slaveId, uint16_t address, uint16_t value) {
    uint8_t request[6];
    request[0] = slaveId;
    request[1] = 0x06;
    request[2] = (uint8_t)(address >> 8);
    request[3] = (uint8_t)(address & 0xFF);
    request[4] = (uint8_t)(value >> 8);
    request[5] = (uint8_t)(value & 0xFF);
    
    return modbusSendRequest(request, 6);
}

// Función 0x10: Write Multiple Registers
ModbusResponse modbusWriteMultipleRegisters(uint8_t slaveId, uint16_t startAddress, uint16_t quantity, uint16_t *values) {
    uint8_t byteCount = quantity * 2;
    uint8_t request[7 + byteCount];
    
    request[0] = slaveId;
    request[1] = 0x10;
    request[2] = (uint8_t)(startAddress >> 8);
    request[3] = (uint8_t)(startAddress & 0xFF);
    request[4] = (uint8_t)(quantity >> 8);
    request[5] = (uint8_t)(quantity & 0xFF);
    request[6] = byteCount;
    
    for (uint16_t i = 0; i < quantity; i++) {
        request[7 + i*2] = (uint8_t)(values[i] >> 8);
        request[7 + i*2 + 1] = (uint8_t)(values[i] & 0xFF);
    }
    
    return modbusSendRequest(request, 7 + byteCount);
}
