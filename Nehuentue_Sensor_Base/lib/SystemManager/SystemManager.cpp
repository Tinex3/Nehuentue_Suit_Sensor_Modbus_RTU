/**
 * @file SystemManager.cpp
 * @brief Implementación del SystemManager
 * @version 1.0.0
 * @date 2025-10-19
 */

#include "SystemManager.h"
#include <WiFi.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <esp_flash.h>
#include <esp_partition.h>
#include <esp_ota_ops.h>

// Instancia global
SystemManager SysMgr;

// ============================================================================
// CONSTRUCTOR
// ============================================================================

SystemManager::SystemManager() {
    initialized = false;
    startTime = 0;
}

SystemManager::~SystemManager() {
}

// ============================================================================
// INICIALIZACIÓN
// ============================================================================

bool SystemManager::begin() {
    if (initialized) {
        return true;
    }
    
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║   System Manager v1.0                  ║");
    Serial.println("║   Nehuentue Suit Sensor                ║");
    Serial.println("╚════════════════════════════════════════╝");
    
    startTime = millis();
    initialized = true;
    
    printInfo();
    
    return true;
}

void SystemManager::loop() {
    // Monitoreo periódico del sistema
    // Aquí se puede agregar watchdog, health checks, etc.
}

// ============================================================================
// ESTADO DEL SISTEMA
// ============================================================================

SystemStatus SystemManager::getStatus() {
    SystemStatus status;
    
    // Estado WiFi (placeholder - se integrará con WiFiManager)
    status.wifiConnected = WiFi.status() == WL_CONNECTED;
    
    // Estado MQTT (placeholder - se integrará con MQTTManager)
    status.mqttConnected = false;
    
    // Estado Modbus
    status.modbusEnabled = true;
    
    // Estado WebServer
    status.webServerRunning = true;
    
    // Información del sistema
    status.uptime = millis() - startTime;
    status.freeHeap = ESP.getFreeHeap();
    status.minFreeHeap = ESP.getMinFreeHeap();
    status.cpuFreqMHz = ESP.getCpuFreqMHz();
    status.firmwareVersion = FW_VERSION;
    status.chipModel = ESP.getChipModel();
    status.chipRevision = ESP.getChipRevision();
    
    return status;
}

FirmwareInfo SystemManager::getFirmwareInfo() {
    FirmwareInfo info;
    info.version = FW_VERSION;
    info.buildDate = FW_BUILD_DATE;
    info.buildTime = FW_BUILD_TIME;
    info.author = FW_AUTHOR;
    info.project = FW_PROJECT;
    return info;
}

// ============================================================================
// REINICIO Y RESET
// ============================================================================

void SystemManager::restart(uint32_t delayMs) {
    Serial.printf("\n[SYSTEM] Reiniciando en %lu ms...\n", delayMs);
    delay(delayMs);
    ESP.restart();
}

void SystemManager::resetConfiguration() {
    Serial.println("[SYSTEM] Reseteando configuración...");
    // Aquí se integrará con FlashStorageManager para limpiar configuración
    Serial.println("[SYSTEM] Configuración limpiada");
}

void SystemManager::factoryReset() {
    Serial.println("\n[SYSTEM] ⚠️  FACTORY RESET ⚠️");
    Serial.println("[SYSTEM] Eliminando toda la configuración...");
    
    resetConfiguration();
    
    Serial.println("[SYSTEM] Factory reset completado");
    restart(2000);
}

// ============================================================================
// UTILIDADES
// ============================================================================

void SystemManager::printInfo() {
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║   Información del Sistema              ║");
    Serial.println("╚════════════════════════════════════════╝");
    
    // Firmware
    Serial.printf("  Firmware: v%s\n", FW_VERSION);
    Serial.printf("  Compilado: %s %s\n", FW_BUILD_DATE, FW_BUILD_TIME);
    Serial.printf("  Proyecto: %s\n", FW_PROJECT);
    Serial.printf("  Autor: %s\n", FW_AUTHOR);
    Serial.println("----------------------------------------");
    
    // Hardware
    Serial.printf("  Chip: %s rev%d\n", ESP.getChipModel(), ESP.getChipRevision());
    Serial.printf("  CPU: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("  Chip ID: %s\n", getChipId().c_str());
    Serial.println("----------------------------------------");
    
    // Memoria
    Serial.printf("  Heap libre: %lu bytes\n", ESP.getFreeHeap());
    Serial.printf("  Heap mínimo: %lu bytes\n", ESP.getMinFreeHeap());
    Serial.printf("  Tamaño sketch: %lu bytes\n", ESP.getSketchSize());
    Serial.printf("  Espacio libre: %lu bytes\n", ESP.getFreeSketchSpace());
    Serial.println("----------------------------------------");
    
    // Boot
    Serial.printf("  Razón boot: %s\n", getBootReason());
    Serial.println("════════════════════════════════════════\n");
}

void SystemManager::printStatus() {
    SystemStatus status = getStatus();
    
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║   Estado del Sistema                   ║");
    Serial.println("╚════════════════════════════════════════╝");
    
    Serial.printf("  WiFi: %s\n", status.wifiConnected ? "✓ Conectado" : "✗ Desconectado");
    Serial.printf("  MQTT: %s\n", status.mqttConnected ? "✓ Conectado" : "✗ Desconectado");
    Serial.printf("  Modbus: %s\n", status.modbusEnabled ? "✓ Habilitado" : "✗ Deshabilitado");
    Serial.printf("  WebServer: %s\n", status.webServerRunning ? "✓ Ejecutando" : "✗ Detenido");
    Serial.println("----------------------------------------");
    
    uint32_t seconds = status.uptime / 1000;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    uint32_t days = hours / 24;
    
    if (days > 0) {
        Serial.printf("  Uptime: %lud %02luh %02lum %02lus\n", 
                     days, hours % 24, minutes % 60, seconds % 60);
    } else {
        Serial.printf("  Uptime: %02luh %02lum %02lus\n", 
                     hours, minutes % 60, seconds % 60);
    }
    
    Serial.printf("  Heap libre: %lu bytes\n", status.freeHeap);
    Serial.printf("  CPU: %.0f MHz\n", status.cpuFreqMHz);
    Serial.println("════════════════════════════════════════\n");
}

const char* SystemManager::getBootReason() {
    esp_reset_reason_t reason = esp_reset_reason();
    
    switch (reason) {
        case ESP_RST_UNKNOWN:   return "Desconocido";
        case ESP_RST_POWERON:   return "Power-on";
        case ESP_RST_EXT:       return "Reset externo";
        case ESP_RST_SW:        return "Reset software";
        case ESP_RST_PANIC:     return "Excepción/panic";
        case ESP_RST_INT_WDT:   return "Watchdog interrupt";
        case ESP_RST_TASK_WDT:  return "Watchdog task";
        case ESP_RST_WDT:       return "Otros watchdog";
        case ESP_RST_DEEPSLEEP: return "Deep sleep";
        case ESP_RST_BROWNOUT:  return "Brownout";
        case ESP_RST_SDIO:      return "Reset SDIO";
        default:                return "Otro";
    }
}

String SystemManager::getChipId() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char chipId[18];
    snprintf(chipId, sizeof(chipId), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(chipId);
}

// ============================================================================
// INFORMACIÓN DE MEMORIA (ESP-IDF APIs)
// ============================================================================

void SystemManager::getMemoryInfo(uint32_t& totalRam, uint32_t& freeRam, uint32_t& usedRam,
                                   uint32_t& minFreeRam, uint32_t& largestFreeBlock) {
    // Obtener información de heap usando ESP-IDF heap capabilities API
    multi_heap_info_t heapInfo;
    heap_caps_get_info(&heapInfo, MALLOC_CAP_DEFAULT);
    
    // Heap total disponible (interno + PSRAM si existe)
    totalRam = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
    
    // Heap libre actual
    freeRam = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    
    // Heap usado
    usedRam = totalRam - freeRam;
    
    // Mínimo heap libre desde el boot (water mark)
    minFreeRam = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
    
    // Mayor bloque contiguo disponible
    largestFreeBlock = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
}

void SystemManager::getFlashInfo(uint32_t& totalFlash, uint32_t& usedFlash, uint32_t& freeFlash,
                                  uint32_t& appSize, uint32_t& otaSize) {
    // Obtener información de la partición actual (running app)
    const esp_partition_t* runningPartition = esp_ota_get_running_partition();
    
    // Tamaño total de flash usando ESP-IDF
    uint32_t flashSize = 0;
    esp_flash_get_size(NULL, &flashSize);
    totalFlash = flashSize;
    
    // Tamaño de la partición de la aplicación actual
    if (runningPartition != NULL) {
        appSize = runningPartition->size;
    } else {
        appSize = 0;
    }
    
    // Tamaño usado por el sketch/firmware actual (binario compilado)
    usedFlash = ESP.getSketchSize();
    
    // Espacio disponible para OTA (next update partition)
    const esp_partition_t* otaPartition = esp_ota_get_next_update_partition(NULL);
    if (otaPartition != NULL) {
        otaSize = otaPartition->size;
        // El espacio libre es lo que queda en la partición OTA
        freeFlash = otaSize;
    } else {
        otaSize = 0;
        // Si no hay partición OTA, usar el espacio libre del sketch
        freeFlash = ESP.getFreeSketchSpace();
    }
}

