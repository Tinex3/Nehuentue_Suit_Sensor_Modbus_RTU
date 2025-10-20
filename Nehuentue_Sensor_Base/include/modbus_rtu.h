#ifndef MODBUS_RTU_H
#define MODBUS_RTU_H

#include <Arduino.h>

// Forward declaration - usar la estructura del ModbusManager
struct ModbusResponse;

// Configuración Modbus RTU Master
#define MODBUS_SERIAL_PORT Serial1
#define MODBUS_TIMEOUT_MS 1000
#define MODBUS_MAX_RESPONSE_SIZE 256

// Funciones públicas para Modbus Master
void modbusRTUInit(int rxPin, int txPin, unsigned long baudrate = 9600);

// Funciones Modbus Master - devuelven la respuesta completa para que la decodifiques
ModbusResponse modbusReadHoldingRegisters(uint8_t slaveId, uint16_t startAddress, uint16_t quantity);
ModbusResponse modbusWriteSingleRegister(uint8_t slaveId, uint16_t address, uint16_t value);
ModbusResponse modbusWriteMultipleRegisters(uint8_t slaveId, uint16_t startAddress, uint16_t quantity, uint16_t *values);
ModbusResponse modbusReadInputRegisters(uint8_t slaveId, uint16_t startAddress, uint16_t quantity);
ModbusResponse modbusReadCoils(uint8_t slaveId, uint16_t startAddress, uint16_t quantity);

// Función genérica para enviar cualquier mensaje Modbus y recibir respuesta
ModbusResponse modbusSendRequest(uint8_t *request, size_t requestLength);

// Utilidades
uint16_t modbusCalculateCRC(const uint8_t *buf, size_t len);
bool modbusVerifyCRC(const uint8_t *buf, size_t len);

#endif // MODBUS_RTU_H
