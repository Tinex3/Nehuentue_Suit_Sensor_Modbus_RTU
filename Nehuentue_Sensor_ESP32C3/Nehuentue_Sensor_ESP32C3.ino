/*
 * Nehuentue Suit Sensor - ESP32-C3 Modbus RTU
 * 
 * Este firmware implementa un sensor Modbus RTU esclavo para ESP32-C3
 * Compatible con Arduino Framework
 * 
 * Características:
 * - Comunicación Modbus RTU por UART
 * - Lectura de sensores (temperatura, humedad, presión)
 * - Configuración mediante registros Modbus
 * - LED de estado
 * 
 * Autor: Tinex3
 * Versión: 1.0.0
 */

#include <Arduino.h>
#include <ModbusRTU.h>
#include "config.h"

// Variables globales
ModbusRTU mb;
uint16_t holdingRegs[NUM_REGISTERS];
unsigned long lastSampleTime = 0;
uint16_t sampleRate = DEFAULT_SAMPLE_RATE;  // Intervalo de muestreo por defecto

// Prototipos de funciones
void setupModbus();
void readSensors();
void updateLED();
uint16_t cbWrite(TRegister* reg, uint16_t val);

void setup() {
  // Inicializar debug si está habilitado
  #if DEBUG_ENABLED
  DEBUG_BEGIN(115200);
  DEBUG_PRINTLN("Nehuentue Sensor ESP32-C3 - Iniciando...");
  #endif
  
  // Inicializar LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Inicializar registros con valores por defecto
  holdingRegs[REG_TEMPERATURA] = 0;
  holdingRegs[REG_HUMEDAD] = 0;
  holdingRegs[REG_PRESION] = 0;
  holdingRegs[REG_DEVICE_ID] = MODBUS_SLAVE_ID;
  holdingRegs[REG_FIRMWARE_VER] = FIRMWARE_VERSION;  // Versión desde config.h
  holdingRegs[REG_STATUS] = 1;          // 1 = OK
  holdingRegs[REG_LED_CONTROL] = 0;     // LED apagado
  holdingRegs[REG_SAMPLE_RATE] = sampleRate;
  
  // Configurar Modbus
  setupModbus();
  
  // Parpadeo inicial del LED para indicar inicio correcto
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
  
  #if DEBUG_ENABLED
  DEBUG_PRINTLN("Sistema iniciado correctamente");
  DEBUG_PRINT("Slave ID: ");
  DEBUG_PRINTLN(MODBUS_SLAVE_ID);
  DEBUG_PRINT("Baudrate: ");
  DEBUG_PRINTLN(MODBUS_BAUDRATE);
  #endif
}

void loop() {
  // Procesar comunicación Modbus
  mb.task();
  
  // Leer sensores según el intervalo configurado
  if (millis() - lastSampleTime >= holdingRegs[REG_SAMPLE_RATE]) {
    readSensors();
    lastSampleTime = millis();
  }
  
  // Actualizar estado del LED
  updateLED();
  
  // Pequeña pausa para evitar saturar la CPU
  delay(10);
}

void setupModbus() {
  // Configurar Serial para Modbus RTU
  Serial1.begin(MODBUS_BAUDRATE, MODBUS_CONFIG, MODBUS_RX_PIN, MODBUS_TX_PIN);
  
  // Configurar pin DE/RE si se usa transceptor RS485
  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);  // Modo recepción por defecto
  
  // Inicializar Modbus RTU
  mb.begin(&Serial1, DE_RE_PIN);
  mb.slave(MODBUS_SLAVE_ID);
  
  // Agregar registros Holding
  mb.addHreg(REG_TEMPERATURA, holdingRegs[REG_TEMPERATURA]);
  mb.addHreg(REG_HUMEDAD, holdingRegs[REG_HUMEDAD]);
  mb.addHreg(REG_PRESION, holdingRegs[REG_PRESION]);
  mb.addHreg(REG_DEVICE_ID, holdingRegs[REG_DEVICE_ID]);
  mb.addHreg(REG_FIRMWARE_VER, holdingRegs[REG_FIRMWARE_VER]);
  mb.addHreg(REG_STATUS, holdingRegs[REG_STATUS]);
  mb.addHreg(REG_LED_CONTROL, holdingRegs[REG_LED_CONTROL]);
  mb.addHreg(REG_SAMPLE_RATE, holdingRegs[REG_SAMPLE_RATE]);
  
  // Callback para escritura de registros
  mb.onSetHreg(REG_LED_CONTROL, cbWrite, NUM_REGISTERS - REG_LED_CONTROL);
}

void readSensors() {
  // Simulación de lectura de sensores
  // En una implementación real, aquí se leerían sensores físicos
  // como DHT22, BME280, DS18B20, etc.
  
  // Ejemplo: Temperatura simulada (20.0°C a 30.0°C)
  float temp = 20.0 + (millis() % 10000) / 1000.0;
  holdingRegs[REG_TEMPERATURA] = (uint16_t)(temp * 100);
  mb.Hreg(REG_TEMPERATURA, holdingRegs[REG_TEMPERATURA]);
  
  // Ejemplo: Humedad simulada (40.0% a 80.0%)
  float hum = 40.0 + (millis() % 8000) / 200.0;
  holdingRegs[REG_HUMEDAD] = (uint16_t)(hum * 100);
  mb.Hreg(REG_HUMEDAD, holdingRegs[REG_HUMEDAD]);
  
  // Ejemplo: Presión simulada (1000 hPa a 1020 hPa)
  uint16_t pres = 1000 + (millis() % 2000) / 100;
  holdingRegs[REG_PRESION] = pres;
  mb.Hreg(REG_PRESION, holdingRegs[REG_PRESION]);
  
  // Actualizar estado (1 = OK)
  holdingRegs[REG_STATUS] = 1;
  mb.Hreg(REG_STATUS, holdingRegs[REG_STATUS]);
}

void updateLED() {
  // Controlar LED según el registro
  if (holdingRegs[REG_LED_CONTROL] > 0) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}

uint16_t cbWrite(TRegister* reg, uint16_t val) {
  // Callback cuando se escriben registros
  uint16_t regNum = reg->address.address;
  
  switch (regNum) {
    case REG_LED_CONTROL:
      holdingRegs[REG_LED_CONTROL] = val;
      #if DEBUG_ENABLED
      DEBUG_PRINT("LED control: ");
      DEBUG_PRINTLN(val);
      #endif
      break;
      
    case REG_SAMPLE_RATE:
      // Limitar el rango de la tasa de muestreo
      if (val >= MIN_SAMPLE_RATE && val <= MAX_SAMPLE_RATE) {
        holdingRegs[REG_SAMPLE_RATE] = val;
        sampleRate = val;
        #if DEBUG_ENABLED
        DEBUG_PRINT("Sample rate: ");
        DEBUG_PRINT(val);
        DEBUG_PRINTLN(" ms");
        #endif
      }
      break;
      
    default:
      // Los demás registros son de solo lectura
      break;
  }
  
  return val;
}
