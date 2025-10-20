#ifndef TASKS_H
#define TASKS_H

#include <Arduino.h>
#include "modbus_rtu.h"
#include "eeprom_manager.h"

// Forward declarations - usar las estructuras de los managers
struct WiFiConfig;
struct MQTTConfig;
struct SensorConfig;
struct ModbusResponse;

// Respuesta Modbus cruda completa (almacena bytes sin procesar)
struct ModbusRawResponse {
    bool valid;                   // Si la lectura fue exitosa
    uint8_t data[256];           // Respuesta Modbus completa (máx 256 bytes)
    uint8_t length;              // Longitud real de la respuesta
    uint8_t slaveAddress;        // Dirección del esclavo que respondió
    uint8_t functionCode;        // Código de función Modbus usado
    uint16_t registerStart;      // Registro inicial consultado
    uint16_t registerCount;      // Cantidad de registros consultados
    unsigned long timestamp;     // Timestamp de la lectura
};

// Estructura para compartir datos entre tareas (1 solo sensor)
struct SensorData {
    ModbusRawResponse modbusResponse;  // Respuesta Modbus cruda
    
    // Campos decodificados (opcionales, para telemetría procesada)
    uint16_t registers[125];     // Hasta 125 registros decodificados (máx Modbus en 1 trama)
    uint8_t registerCount;       // Cantidad de registros decodificados
    bool valid;                  // Si los datos decodificados son válidos
    unsigned long timestamp;     // Timestamp de decodificación
};

// Tópicos MQTT (se construyen dinámicamente con deviceId)
struct MQTTTopics {
    char telemetry[64];          // devices/{deviceId}/telemetry (datos decodificados)
    char modbusRaw[64];          // devices/{deviceId}/modbus/response (respuesta cruda)
    char status[64];             // devices/{deviceId}/status
    char eventError[64];         // devices/{deviceId}/event/error
    char eventConnect[64];       // devices/{deviceId}/event/connect
    char cmdBase[64];            // devices/{deviceId}/cmd/#
};

// Comandos MQTT que puede recibir el dispositivo
enum MQTTCommand {
    MQTT_CMD_RESET,
    MQTT_CMD_RECALIBRATE,
    MQTT_CMD_UPDATE_CONFIG,
    MQTT_CMD_GET_STATUS
};

// Estructura para guardar datos del sensor en EEPROM
struct StoredSensorData {
    float temperature;
    float humidity;
    unsigned long timestamp;
    uint16_t crc;  // Para verificación de integridad
};

// Comandos para la cola de EEPROM
enum EEPROMCommand {
    EEPROM_CMD_WRITE_SENSOR_DATA,
    EEPROM_CMD_WRITE_CONFIG,
    EEPROM_CMD_READ_CONFIG
};

// Estructura para peticiones a la tarea EEPROM
struct EEPROMRequest {
    EEPROMCommand command;
    void* data;  // Puntero a datos (opcional)
};

// Direcciones de memoria EEPROM (layout de memoria)
#define EEPROM_ADDR_WIFI_CONFIG 0        // WiFiConfig en 0
#define EEPROM_ADDR_SENSOR_CONFIG 256    // SensorConfig en 256 (1 solo sensor)
#define EEPROM_ADDR_MQTT_CONFIG 512      // MQTTConfig en 512

// Variables compartidas (protegidas por mutex)
extern SensorData sensorData;
extern SemaphoreHandle_t dataMutex;
extern WiFiConfig wifiConfig;
extern MQTTTopics mqttTopics;
extern MQTTConfig mqttConfig;
extern SensorConfig sensorConfig;  // 1 solo sensor (no array)
extern QueueHandle_t eepromQueue;

// Funciones de tareas FreeRTOS
void modbusTask(void *pvParameters);
void decoderTask(void *pvParameters);
void mqttTask(void *pvParameters);      // Nueva tarea dedicada a MQTT
// void eepromTask(void *pvParameters);  // DESHABILITADA - sin hardware

// Funciones auxiliares
void initTasks();
void initDefaultConfig();                // Inicializa configuración sin EEPROM
void buildMQTTTopics(const char* deviceId);  // Construye los tópicos MQTT

#endif // TASKS_H
