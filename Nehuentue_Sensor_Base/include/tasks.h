#ifndef TASKS_H
#define TASKS_H

#include <Arduino.h>
#include "modbus_rtu.h"
#include "eeprom_manager.h"

// Estructura para compartir datos entre tareas
struct SensorData {
    bool valid;
    uint16_t register0;
    uint16_t register1;
    float temperature;
    float humidity;
    unsigned long timestamp;
};

// Configuración WiFi/MQTT
struct WiFiConfig {
    char ssid[32];
    char password[64];
    char mqttServer[64];
    uint16_t mqttPort;
    char mqttUser[32];
    char mqttPassword[64];
    char deviceId[32];  // ID del dispositivo (ej: "modbus-01")
};

// Tópicos MQTT (se construyen dinámicamente con deviceId)
struct MQTTTopics {
    char telemetryTemp[64];      // devices/{deviceId}/telemetry/temperature
    char telemetryCurrent[64];   // devices/{deviceId}/telemetry/current
    char status[64];             // devices/{deviceId}/status
    char eventError[64];         // devices/{deviceId}/event/error
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
#define EEPROM_ADDR_SENSOR_DATA 256      // StoredSensorData en 256

// Variables compartidas (protegidas por mutex)
extern SensorData sensorData;
extern SemaphoreHandle_t dataMutex;
extern WiFiConfig wifiConfig;
extern MQTTTopics mqttTopics;
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
