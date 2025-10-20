/**
 * @file config.h
 * @brief Configuración del sistema Nehuentue Sensor
 * @version 2.0.0
 * @date 2025-10-19
 * 
 * NOTA: Las estructuras WiFiConfig y MQTTConfig están definidas
 * en los respectivos managers (WiFiManager.h y MQTTManager.h)
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Versión de configuración (para migración)
#define CONFIG_VERSION 1

// ============================================================================
// Sensor Configuration (única estructura no definida en managers)
// ============================================================================
struct SensorConfig {
  char name[32];              // Nombre del sensor
  char type[32];              // Tipo de sensor
  char unit[16];              // Unidad de medida
  
  bool enabled;
  uint8_t slaveId;            // = modbusAddress (compatibilidad)
  uint8_t modbusAddress;      // Alias para slaveId
  uint8_t modbusFunction;     // Función Modbus (0x03, 0x04, etc.)
  uint16_t startAddress;
  uint16_t registerStart;     // Alias para startAddress
  uint16_t quantity;
  uint16_t registerCount;     // Alias para quantity
  uint32_t pollInterval;
  int rxPin;
  int txPin;
  uint32_t baudrate;
  
  // Campos para conversión de datos
  float multiplier;
  float offset;
  uint8_t decimals;
  
  uint8_t version;
  
  SensorConfig() {
    memset(this, 0, sizeof(SensorConfig));
    version = CONFIG_VERSION;
    enabled = true;
    slaveId = 1;
    modbusAddress = 1;
    modbusFunction = 0x03;  // Read Holding Registers por defecto
    startAddress = 0;
    registerStart = 0;
    quantity = 10;
    registerCount = 10;
    multiplier = 1.0f;
    offset = 0.0f;
    decimals = 2;
    pollInterval = 1000;
    rxPin = 20;
    txPin = 21;
    baudrate = 9600;
    strcpy(name, "Sensor 1");
  }
};

// ============================================================================
// System Statistics
// ============================================================================
struct SystemStats {
  uint32_t successfulReads;
  uint32_t failedReads;
  uint32_t mqttPublished;
  uint32_t wifiReconnects;
  
  SystemStats() {
    memset(this, 0, sizeof(SystemStats));
  }
};

// ============================================================================
// Configuración de pines por defecto
// ============================================================================
#define DEFAULT_MODBUS_RX_PIN     20
#define DEFAULT_MODBUS_TX_PIN     21
#define DEFAULT_MODBUS_BAUDRATE   9600

// ============================================================================
// CREDENCIALES PRECONFIGURADAS (pueden ser sobrescritas por FlashStorage)
// ============================================================================

// WiFi
#define DEFAULT_WIFI_SSID         "Amanda 2.4G"
#define DEFAULT_WIFI_PASSWORD     "Gomezriquelmegomez12"
#define DEFAULT_HOSTNAME          "Nehuentue-Sensor"

// MQTT Broker
#define DEFAULT_MQTT_SERVER       "192.168.1.25"  // Raspberry Pi con Mosquitto
#define DEFAULT_MQTT_PORT         1883
#define DEFAULT_MQTT_USER         "mqttuser"
#define DEFAULT_MQTT_PASSWORD     "1234"
#define DEFAULT_MQTT_CLIENT_ID    "nehuentue_sensor_001"

// Tópicos MQTT
#define MQTT_TOPIC_BASE           "nehuentue"
#define MQTT_TOPIC_TELEMETRY      "telemetry"
#define MQTT_TOPIC_STATUS         "status"
#define MQTT_TOPIC_CMD            "cmd"
#define MQTT_TOPIC_RESPONSE       "response"

// Intervalos (milisegundos)
#define DEFAULT_TELEMETRY_INTERVAL  60000   // 60 segundos
#define DEFAULT_STATUS_INTERVAL     300000  // 5 minutos

// ============================================================================
// SISTEMA DE CÓDIGOS DE ERROR
// ============================================================================

// Tipos de error (error_type)
enum ErrorType {
    ERROR_NONE = 0,
    ERROR_WIFI = 1,
    ERROR_MQTT = 2,
    ERROR_MODBUS = 3,
    ERROR_EEPROM = 4,
    ERROR_FLASH = 5,
    ERROR_SENSOR = 6,
    ERROR_SYSTEM = 7,
    ERROR_MEMORY = 8,
    ERROR_NETWORK = 9,
    ERROR_UNKNOWN = 99
};

// Códigos de error específicos (error_code)
enum ErrorCode {
    // Sin error
    ERR_NONE = 0,
    
    // Errores WiFi (100-199)
    ERR_WIFI_DISCONNECTED = 100,
    ERR_WIFI_CONNECTION_FAILED = 101,
    ERR_WIFI_WEAK_SIGNAL = 102,
    ERR_WIFI_AUTH_FAILED = 103,
    ERR_WIFI_NO_SSID = 104,
    ERR_WIFI_TIMEOUT = 105,
    
    // Errores MQTT (200-299)
    ERR_MQTT_DISCONNECTED = 200,
    ERR_MQTT_CONNECTION_FAILED = 201,
    ERR_MQTT_PUBLISH_FAILED = 202,
    ERR_MQTT_SUBSCRIBE_FAILED = 203,
    ERR_MQTT_BROKER_UNREACHABLE = 204,
    ERR_MQTT_AUTH_FAILED = 205,
    
    // Errores Modbus (300-399)
    ERR_MODBUS_NO_RESPONSE = 300,
    ERR_MODBUS_TIMEOUT = 301,
    ERR_MODBUS_CRC_ERROR = 302,
    ERR_MODBUS_EXCEPTION = 303,
    ERR_MODBUS_INVALID_SLAVE = 304,
    ERR_MODBUS_INVALID_FUNCTION = 305,
    ERR_MODBUS_INVALID_ADDRESS = 306,
    ERR_MODBUS_COMMUNICATION_ERROR = 307,
    
    // Errores EEPROM/Flash (400-499)
    ERR_EEPROM_INIT_FAILED = 400,
    ERR_EEPROM_READ_FAILED = 401,
    ERR_EEPROM_WRITE_FAILED = 402,
    ERR_EEPROM_CRC_MISMATCH = 403,
    ERR_FLASH_FULL = 404,
    ERR_FLASH_CORRUPTED = 405,
    
    // Errores de Sensor (500-599)
    ERR_SENSOR_NOT_CONFIGURED = 500,
    ERR_SENSOR_INVALID_DATA = 501,
    ERR_SENSOR_OUT_OF_RANGE = 502,
    ERR_SENSOR_CALIBRATION_ERROR = 503,
    
    // Errores de Sistema (600-699)
    ERR_SYSTEM_LOW_MEMORY = 600,
    ERR_SYSTEM_HEAP_FRAGMENTED = 601,
    ERR_SYSTEM_WATCHDOG = 602,
    ERR_SYSTEM_BOOT_FAILED = 603,
    ERR_SYSTEM_TASK_FAILED = 604,
    
    // Errores de Red (700-799)
    ERR_NETWORK_NO_GATEWAY = 700,
    ERR_NETWORK_DNS_FAILED = 701,
    ERR_NETWORK_PING_FAILED = 702,
    
    // Error desconocido
    ERR_UNKNOWN = 999
};

// Estructura de error
struct SystemError {
    ErrorType type;
    ErrorCode code;
    char description[128];
    unsigned long timestamp;
    bool active;
    
    SystemError() {
        type = ERROR_NONE;
        code = ERR_NONE;
        memset(description, 0, sizeof(description));
        timestamp = 0;
        active = false;
    }
};

// Función para obtener nombre del tipo de error
inline const char* getErrorTypeName(ErrorType type) {
    switch (type) {
        case ERROR_NONE: return "NONE";
        case ERROR_WIFI: return "WIFI";
        case ERROR_MQTT: return "MQTT";
        case ERROR_MODBUS: return "MODBUS";
        case ERROR_EEPROM: return "EEPROM";
        case ERROR_FLASH: return "FLASH";
        case ERROR_SENSOR: return "SENSOR";
        case ERROR_SYSTEM: return "SYSTEM";
        case ERROR_MEMORY: return "MEMORY";
        case ERROR_NETWORK: return "NETWORK";
        case ERROR_UNKNOWN: return "UNKNOWN";
        default: return "UNDEFINED";
    }
}

// Función para obtener descripción del código de error
inline const char* getErrorDescription(ErrorCode code) {
    switch (code) {
        case ERR_NONE: return "Sin errores";
        
        // WiFi
        case ERR_WIFI_DISCONNECTED: return "WiFi desconectado";
        case ERR_WIFI_CONNECTION_FAILED: return "Fallo al conectar a WiFi";
        case ERR_WIFI_WEAK_SIGNAL: return "Señal WiFi débil";
        case ERR_WIFI_AUTH_FAILED: return "Autenticación WiFi fallida";
        case ERR_WIFI_NO_SSID: return "SSID no encontrado";
        case ERR_WIFI_TIMEOUT: return "Timeout de conexión WiFi";
        
        // MQTT
        case ERR_MQTT_DISCONNECTED: return "MQTT desconectado";
        case ERR_MQTT_CONNECTION_FAILED: return "Fallo al conectar a broker MQTT";
        case ERR_MQTT_PUBLISH_FAILED: return "Fallo al publicar mensaje MQTT";
        case ERR_MQTT_SUBSCRIBE_FAILED: return "Fallo al suscribirse a tópico";
        case ERR_MQTT_BROKER_UNREACHABLE: return "Broker MQTT inaccesible";
        case ERR_MQTT_AUTH_FAILED: return "Autenticación MQTT fallida";
        
        // Modbus
        case ERR_MODBUS_NO_RESPONSE: return "Esclavo Modbus no responde";
        case ERR_MODBUS_TIMEOUT: return "Timeout en comunicación Modbus";
        case ERR_MODBUS_CRC_ERROR: return "Error CRC en Modbus";
        case ERR_MODBUS_INVALID_SLAVE: return "Dirección de esclavo inválida";
        case ERR_MODBUS_INVALID_FUNCTION: return "Función Modbus no soportada";
        case ERR_MODBUS_INVALID_ADDRESS: return "Dirección de registro inválida";
        case ERR_MODBUS_COMMUNICATION_ERROR: return "Error de comunicación Modbus";
        
        // EEPROM/Flash
        case ERR_EEPROM_INIT_FAILED: return "Fallo al inicializar EEPROM";
        case ERR_EEPROM_READ_FAILED: return "Error al leer EEPROM";
        case ERR_EEPROM_WRITE_FAILED: return "Error al escribir EEPROM";
        case ERR_EEPROM_CRC_MISMATCH: return "CRC de EEPROM no coincide";
        case ERR_FLASH_FULL: return "Memoria Flash llena";
        case ERR_FLASH_CORRUPTED: return "Flash corrupta";
        
        // Sensor
        case ERR_SENSOR_NOT_CONFIGURED: return "Sensor no configurado";
        case ERR_SENSOR_INVALID_DATA: return "Datos de sensor inválidos";
        case ERR_SENSOR_OUT_OF_RANGE: return "Valor fuera de rango";
        case ERR_SENSOR_CALIBRATION_ERROR: return "Error de calibración";
        
        // Sistema
        case ERR_SYSTEM_LOW_MEMORY: return "Memoria baja";
        case ERR_SYSTEM_HEAP_FRAGMENTED: return "Heap fragmentado";
        case ERR_SYSTEM_WATCHDOG: return "Watchdog activado";
        case ERR_SYSTEM_BOOT_FAILED: return "Fallo al iniciar";
        case ERR_SYSTEM_TASK_FAILED: return "Tarea FreeRTOS fallida";
        
        // Red
        case ERR_NETWORK_NO_GATEWAY: return "Gateway no disponible";
        case ERR_NETWORK_DNS_FAILED: return "Resolución DNS fallida";
        case ERR_NETWORK_PING_FAILED: return "Ping fallido";
        
        default: return "Error desconocido";
    }
}

#endif // CONFIG_H
