#include <Arduino.h>
#include "modbus_rtu.h"
#include "tasks.h"

void setup() {
  // Inicializa serial para debug
  Serial.begin(9600);
  delay(1000);
  Serial.println("==============================================");
  Serial.println("  Nehuentue Sensor Base - Modbus Master");
  Serial.println("  Arquitectura: 4 Tareas FreeRTOS");
  Serial.println("==============================================\n");
  
  // Inicializa Modbus RTU Master con pines y baudrate personalizados
  modbusRTUInit(20, 21, 9600);  // RX=GPIO20, TX=GPIO21, 9600 bps
  
  // ========== EEPROM DESHABILITADA TEMPORALMENTE ==========
  // Inicializa EEPROM 24LC128 (I2C) - API ultra-genérica
  // Parámetros: SDA, SCL, tamaño (16384 para 24LC128), frecuencia, dirección I2C
  // EEPROMStatus eepromStatus = EEPROM24LC64.begin(8, 9, 16384, 100000, 0x50);
  // if (eepromStatus != EEPROM_OK) {
  //   Serial.println("ERROR: No se pudo inicializar EEPROM");
  // }
  Serial.println("[INFO] EEPROM deshabilitada (sin hardware físico)");
  // ========================================================
  
  // Inicializa las 3 tareas FreeRTOS (Modbus, Decoder, MQTT)
  initTasks();
  
  Serial.println("\n==============================================");
  Serial.println("  Sistema iniciado correctamente");
  Serial.println("  Tareas ejecutándose: Modbus + Decoder + MQTT");
  Serial.println("==============================================\n");
}

void loop() {
  // El loop puede estar vacío o mostrar estadísticas
  static unsigned long lastStats = 0;
  if (millis() - lastStats > 30000) {  // Cada 30 segundos
    lastStats = millis();
    
    Serial.println("\n--- Estadísticas del Sistema ---");
    Serial.printf("Uptime: %lu segundos\n", millis() / 1000);
    Serial.printf("Free Heap: %u bytes\n", ESP.getFreeHeap());
    
    // Muestra datos actuales del sensor
    if (dataMutex != NULL && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      if (sensorData.valid) {
        Serial.println("Últimos datos del sensor:");
        Serial.printf("  Temperatura: %.1f °C\n", sensorData.temperature);
        Serial.printf("  Humedad: %.1f %%\n", sensorData.humidity);
        Serial.printf("  Hace: %lu ms\n", millis() - sensorData.timestamp);
      } else {
        Serial.println("No hay datos válidos del sensor");
      }
      xSemaphoreGive(dataMutex);
    }
    Serial.println("--------------------------------\n");
  }
  
  delay(1000);
}