/*
 * config.h - Archivo de configuración para Nehuentue Sensor ESP32-C3
 * 
 * Este archivo permite personalizar la configuración del sensor
 * sin modificar el código principal
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// CONFIGURACIÓN DE PINES
// ============================================================================

// Pines UART para Modbus RTU (ESP32-C3)
#define MODBUS_RX_PIN    20  // GPIO20 - Recepción UART1
#define MODBUS_TX_PIN    21  // GPIO21 - Transmisión UART1
#define DE_RE_PIN        10  // GPIO10 - Control DE/RE del MAX485

// Pin del LED de estado
#define LED_PIN          8   // GPIO8 - LED integrado del ESP32-C3

// Pines para sensores (opcional - descomentar si se usan)
//#define DHT_PIN          4   // GPIO4 - Sensor DHT22
//#define DHT_TYPE         DHT22
//#define DS18B20_PIN      5   // GPIO5 - Sensor DS18B20
//#define I2C_SDA          6   // GPIO6 - I2C SDA (BME280, etc.)
//#define I2C_SCL          7   // GPIO7 - I2C SCL (BME280, etc.)

// ============================================================================
// CONFIGURACIÓN MODBUS RTU
// ============================================================================

// ID del esclavo Modbus (1-247)
#define MODBUS_SLAVE_ID  1

// Velocidad de comunicación (baudios)
// Valores comunes: 9600, 19200, 38400, 57600, 115200
#define MODBUS_BAUDRATE  9600

// Configuración de trama serial
// SERIAL_8N1 = 8 bits datos, sin paridad, 1 bit stop
// SERIAL_8E1 = 8 bits datos, paridad par, 1 bit stop
// SERIAL_8O1 = 8 bits datos, paridad impar, 1 bit stop
#define MODBUS_CONFIG    SERIAL_8N1

// ============================================================================
// CONFIGURACIÓN DE SENSORES
// ============================================================================

// Intervalo de muestreo por defecto (milisegundos)
#define DEFAULT_SAMPLE_RATE  1000  // 1 segundo

// Rango permitido para tasa de muestreo
#define MIN_SAMPLE_RATE      100   // 100ms mínimo
#define MAX_SAMPLE_RATE      10000 // 10 segundos máximo

// Habilitar/deshabilitar sensores simulados
#define ENABLE_SIMULATED_SENSORS  1  // 1 = Activado, 0 = Desactivado

// ============================================================================
// CONFIGURACIÓN DE REGISTROS MODBUS
// ============================================================================

// Direcciones de registros Modbus (Holding Registers)
#define REG_TEMPERATURA     0    // Temperatura x100 (°C)
#define REG_HUMEDAD         1    // Humedad x100 (%)
#define REG_PRESION         2    // Presión (hPa)
#define REG_DEVICE_ID       3    // ID del dispositivo
#define REG_FIRMWARE_VER    4    // Versión del firmware x100
#define REG_STATUS          5    // Estado del sensor (1=OK, 0=Error)
#define REG_LED_CONTROL     6    // Control del LED (0=OFF, 1=ON)
#define REG_SAMPLE_RATE     7    // Tasa de muestreo (ms)
#define NUM_REGISTERS       8    // Número total de registros

// ============================================================================
// INFORMACIÓN DEL FIRMWARE
// ============================================================================

#define FIRMWARE_VERSION_MAJOR  1
#define FIRMWARE_VERSION_MINOR  0
#define FIRMWARE_VERSION_PATCH  0

// Versión codificada (ej: 1.0.0 = 100)
#define FIRMWARE_VERSION  ((FIRMWARE_VERSION_MAJOR * 100) + \
                          (FIRMWARE_VERSION_MINOR * 10) + \
                          FIRMWARE_VERSION_PATCH)

// ============================================================================
// CONFIGURACIÓN DE DEBUG
// ============================================================================

// Habilitar debug por Serial USB (0 = desactivado, 1 = activado)
// Nota: Si se activa, usar Serial (USB) en lugar de Serial1 (UART1)
#define DEBUG_ENABLED    0

#if DEBUG_ENABLED
  #define DEBUG_PRINT(x)    Serial.print(x)
  #define DEBUG_PRINTLN(x)  Serial.println(x)
  #define DEBUG_BEGIN(x)    Serial.begin(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_BEGIN(x)
#endif

// ============================================================================
// CONFIGURACIÓN AVANZADA
// ============================================================================

// Timeout de watchdog (segundos) - 0 para desactivar
#define WATCHDOG_TIMEOUT  0

// Habilitar LED de heartbeat (parpadeo periódico)
#define ENABLE_HEARTBEAT  0

// Intervalo de heartbeat (milisegundos)
#define HEARTBEAT_INTERVAL  1000

// ============================================================================
// VALORES POR DEFECTO DE SENSORES
// ============================================================================

// Valores de sensor cuando no hay hardware conectado
#define DEFAULT_TEMPERATURE  25.0  // °C
#define DEFAULT_HUMIDITY     50.0  // %
#define DEFAULT_PRESSURE     1013  // hPa

#endif // CONFIG_H
