/**
 * @file ModbusManager.h
 * @brief Gestor Modbus RTU Master thread-safe para ESP32
 * @version 1.0.0
 * @date 2025-10-19
 * 
 * Librería profesional para gestión de comunicación Modbus RTU Master.
 * Características:
 * - Thread-safe (FreeRTOS mutex)
 * - Soporte funciones 0x01, 0x03, 0x04, 0x06, 0x10
 * - Cola de peticiones con FreeRTOS
 * - Callbacks para respuestas
 * - CRC16 automático
 * - Estadísticas de comunicación
 * - Detección de excepciones Modbus
 */

#ifndef MODBUS_MANAGER_H
#define MODBUS_MANAGER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <HardwareSerial.h>

// ============================================================================
// CONFIGURACIÓN
// ============================================================================

#define MODBUS_MGR_TIMEOUT_MS         1000    // Timeout petición
#define MODBUS_MGR_MAX_RESPONSE_SIZE  256     // Tamaño máximo respuesta
#define MODBUS_MGR_QUEUE_SIZE         10      // Tamaño cola peticiones
#define MODBUS_MGR_TASK_STACK         4096    // Stack tarea FreeRTOS
#define MODBUS_MGR_TASK_PRIORITY      2       // Prioridad tarea

// ============================================================================
// ESTRUCTURAS
// ============================================================================

/**
 * @brief Estructura de respuesta Modbus
 */
struct ModbusResponse {
    uint8_t data[MODBUS_MGR_MAX_RESPONSE_SIZE];  ///< Datos recibidos
    size_t length;                                ///< Longitud respuesta
    bool success;                                 ///< Operación exitosa
    uint8_t exceptionCode;                        ///< Código excepción (si aplica)
    uint8_t slaveId;                              ///< ID del esclavo
    uint8_t functionCode;                         ///< Código de función
    uint32_t timestamp;                           ///< Timestamp de la respuesta
};

/**
 * @brief Configuración del puerto Modbus
 */
struct ModbusConfig {
    HardwareSerial* serial;     ///< Puerto serial
    int rxPin;                  ///< Pin RX
    int txPin;                  ///< Pin TX
    unsigned long baudrate;     ///< Velocidad (bps)
    uint32_t timeout;           ///< Timeout en ms
};

/**
 * @brief Estadísticas de comunicación Modbus
 */
struct ModbusStats {
    uint32_t totalRequests;       ///< Total peticiones enviadas
    uint32_t successfulRequests;  ///< Peticiones exitosas
    uint32_t failedRequests;      ///< Peticiones fallidas
    uint32_t timeouts;            ///< Timeouts
    uint32_t crcErrors;           ///< Errores de CRC
    uint32_t exceptions;          ///< Excepciones Modbus
    uint32_t lastRequestTime;     ///< Timestamp última petición
    uint32_t lastResponseTime;    ///< Timestamp última respuesta
};

/**
 * @brief Tipo de petición Modbus
 */
enum ModbusRequestType {
    MODBUS_READ_COILS = 0x01,
    MODBUS_READ_DISCRETE_INPUTS = 0x02,
    MODBUS_READ_HOLDING_REGISTERS = 0x03,
    MODBUS_READ_INPUT_REGISTERS = 0x04,
    MODBUS_WRITE_SINGLE_COIL = 0x05,
    MODBUS_WRITE_SINGLE_REGISTER = 0x06,
    MODBUS_WRITE_MULTIPLE_COILS = 0x0F,
    MODBUS_WRITE_MULTIPLE_REGISTERS = 0x10
};

/**
 * @brief Estructura de petición Modbus (interna)
 */
struct ModbusRequest {
    uint8_t slaveId;
    ModbusRequestType type;
    uint16_t startAddress;
    uint16_t quantity;
    uint16_t values[125];  // Max 125 registros
    size_t valueCount;
};

// ============================================================================
// CALLBACKS
// ============================================================================

/**
 * @brief Callback para respuesta Modbus
 * @param response Respuesta recibida
 */
typedef void (*ModbusResponseCallback)(const ModbusResponse& response);

// ============================================================================
// CLASE MODBUSMANAGER
// ============================================================================

class ModbusManager {
public:
    // Constructor y destructor
    ModbusManager();
    ~ModbusManager();
    
    // ========================================================================
    // INICIALIZACIÓN
    // ========================================================================
    
    /**
     * @brief Inicializar Modbus RTU Master
     * @param serial Puerto serial a usar
     * @param rxPin Pin RX
     * @param txPin Pin TX
     * @param baudrate Velocidad en bps (default: 9600)
     * @return true si inicialización exitosa
     */
    bool begin(HardwareSerial& serial, int rxPin, int txPin, unsigned long baudrate = 9600);
    
    /**
     * @brief Finalizar y liberar recursos
     */
    void end();
    
    // ========================================================================
    // FUNCIONES MODBUS
    // ========================================================================
    
    /**
     * @brief Leer Holding Registers (0x03)
     * @param slaveId ID del esclavo (1-247)
     * @param startAddress Dirección inicial
     * @param quantity Cantidad de registros (1-125)
     * @return Respuesta Modbus
     */
    ModbusResponse readHoldingRegisters(uint8_t slaveId, uint16_t startAddress, uint16_t quantity);
    
    /**
     * @brief Leer Input Registers (0x04)
     * @param slaveId ID del esclavo (1-247)
     * @param startAddress Dirección inicial
     * @param quantity Cantidad de registros (1-125)
     * @return Respuesta Modbus
     */
    ModbusResponse readInputRegisters(uint8_t slaveId, uint16_t startAddress, uint16_t quantity);
    
    /**
     * @brief Leer Coils (0x01)
     * @param slaveId ID del esclavo (1-247)
     * @param startAddress Dirección inicial
     * @param quantity Cantidad de coils (1-2000)
     * @return Respuesta Modbus
     */
    ModbusResponse readCoils(uint8_t slaveId, uint16_t startAddress, uint16_t quantity);
    
    /**
     * @brief Escribir un registro (0x06)
     * @param slaveId ID del esclavo (1-247)
     * @param address Dirección del registro
     * @param value Valor a escribir
     * @return Respuesta Modbus
     */
    ModbusResponse writeSingleRegister(uint8_t slaveId, uint16_t address, uint16_t value);
    
    /**
     * @brief Escribir múltiples registros (0x10)
     * @param slaveId ID del esclavo (1-247)
     * @param startAddress Dirección inicial
     * @param quantity Cantidad de registros
     * @param values Array de valores
     * @return Respuesta Modbus
     */
    ModbusResponse writeMultipleRegisters(uint8_t slaveId, uint16_t startAddress, uint16_t quantity, uint16_t* values);
    
    // ========================================================================
    // UTILIDADES
    // ========================================================================
    
    /**
     * @brief Calcular CRC16 Modbus
     * @param buf Buffer de datos
     * @param len Longitud del buffer
     * @return CRC16 calculado
     */
    static uint16_t calculateCRC(const uint8_t* buf, size_t len);
    
    /**
     * @brief Verificar CRC16 de un mensaje
     * @param buf Buffer con CRC al final
     * @param len Longitud total incluyendo CRC
     * @return true si CRC válido
     */
    static bool verifyCRC(const uint8_t* buf, size_t len);
    
    /**
     * @brief Extraer registros de una respuesta
     * @param response Respuesta Modbus
     * @param registers Buffer para almacenar registros
     * @param maxRegisters Tamaño máximo del buffer
     * @return Cantidad de registros extraídos
     */
    static uint16_t extractRegisters(const ModbusResponse& response, uint16_t* registers, size_t maxRegisters);
    
    /**
     * @brief Obtener descripción de excepción
     * @param exceptionCode Código de excepción
     * @return Descripción de la excepción
     */
    static const char* getExceptionDescription(uint8_t exceptionCode);
    
    // ========================================================================
    // CALLBACKS
    // ========================================================================
    
    /**
     * @brief Registrar callback para respuestas
     * @param callback Función callback
     */
    void onResponse(ModbusResponseCallback callback);
    
    // ========================================================================
    // CONFIGURACIÓN
    // ========================================================================
    
    /**
     * @brief Establecer timeout para peticiones
     * @param timeout Timeout en milisegundos
     */
    void setTimeout(uint32_t timeout);
    
    /**
     * @brief Obtener timeout configurado
     * @return Timeout en milisegundos
     */
    uint32_t getTimeout() const { return config.timeout; }
    
    // ========================================================================
    // ESTADÍSTICAS
    // ========================================================================
    
    /**
     * @brief Obtener estadísticas
     * @return Referencia a estadísticas
     */
    const ModbusStats& getStats() const { return stats; }
    
    /**
     * @brief Resetear estadísticas
     */
    void resetStats();
    
    /**
     * @brief Imprimir estadísticas
     */
    void printStats();
    
    /**
     * @brief Imprimir información del manager
     */
    void printInfo();
    
    // ========================================================================
    // ESTADO
    // ========================================================================
    
    /**
     * @brief Verificar si está inicializado
     * @return true si inicializado
     */
    bool isInitialized() const { return initialized; }

private:
    // Configuración
    ModbusConfig config;
    bool initialized;
    
    // FreeRTOS
    SemaphoreHandle_t mutex;
    TaskHandle_t taskHandle;
    QueueHandle_t requestQueue;
    
    // Estadísticas
    ModbusStats stats;
    
    // Callbacks
    ModbusResponseCallback responseCallback;
    
    // Métodos privados
    void lock();
    void unlock();
    ModbusResponse sendRequest(uint8_t* request, size_t length);
    void processRequest(const ModbusRequest& req);
    
    // Tarea FreeRTOS
    static void modbusTask(void* parameter);
};

// Instancia global
extern ModbusManager ModbusMgr;

#endif // MODBUS_MANAGER_H
