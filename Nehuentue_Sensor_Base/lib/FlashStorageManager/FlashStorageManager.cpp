/**
 * @file FlashStorageManager.cpp
 * @brief Implementación del FlashStorageManager
 * @version 1.0.0
 * @date 2025-10-19
 */

#include "FlashStorageManager.h"

// Instancia global
FlashStorageManager FlashStorage;

// ============================================================================
// CONSTRUCTOR Y DESTRUCTOR
// ============================================================================

FlashStorageManager::FlashStorageManager() {
    initialized = false;
    readOnly = false;
    mutex = NULL;
    memset(namespaceName, 0, sizeof(namespaceName));
    memset(&stats, 0, sizeof(FlashStorageStats));
}

FlashStorageManager::~FlashStorageManager() {
    end();
}

// ============================================================================
// INICIALIZACIÓN
// ============================================================================

FlashStorageStatus FlashStorageManager::begin(const char* ns, bool ro) {
    if (initialized) {
        Serial.println("[FLASH STORAGE] Ya inicializado");
        return FLASH_STORAGE_OK;
    }
    
    if (ns == NULL || strlen(ns) == 0) {
        Serial.println("[FLASH STORAGE] ERROR: Namespace inválido");
        return FLASH_STORAGE_ERROR_NOT_INITIALIZED;
    }
    
    if (strlen(ns) > FLASH_STORAGE_MAX_KEY_LENGTH) {
        Serial.println("[FLASH STORAGE] ERROR: Namespace demasiado largo");
        return FLASH_STORAGE_ERROR_KEY_TOO_LONG;
    }
    
    // Crear mutex
    mutex = xSemaphoreCreateMutex();
    if (mutex == NULL) {
        Serial.println("[FLASH STORAGE] ERROR: No se pudo crear mutex");
        return FLASH_STORAGE_ERROR_NOT_INITIALIZED;
    }
    
    // Guardar configuración
    strncpy(namespaceName, ns, sizeof(namespaceName) - 1);
    readOnly = ro;
    
    // Abrir Preferences
    if (!preferences.begin(namespaceName, readOnly)) {
        Serial.println("[FLASH STORAGE] ERROR: No se pudo abrir Preferences");
        vSemaphoreDelete(mutex);
        mutex = NULL;
        return FLASH_STORAGE_ERROR_NOT_INITIALIZED;
    }
    
    initialized = true;
    
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║   Flash Storage Manager v1.0           ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.printf("  Namespace: %s\n", namespaceName);
    Serial.printf("  Modo: %s\n", readOnly ? "Solo lectura" : "Lectura/Escritura");
    Serial.printf("  Thread-safe: ✓\n");
    Serial.printf("  CRC16: ✓\n");
    Serial.printf("  Versioning: ✓\n");
    Serial.println("════════════════════════════════════════\n");
    
    return FLASH_STORAGE_OK;
}

void FlashStorageManager::end() {
    if (initialized) {
        preferences.end();
        initialized = false;
        Serial.println("[FLASH STORAGE] Finalizado");
    }
    
    if (mutex != NULL) {
        vSemaphoreDelete(mutex);
        mutex = NULL;
    }
}

// ============================================================================
// OPERACIONES CON STRINGS
// ============================================================================

FlashStorageStatus FlashStorageManager::saveString(const char* key, const String& value) {
    if (!initialized) return FLASH_STORAGE_ERROR_NOT_INITIALIZED;
    if (strlen(key) > FLASH_STORAGE_MAX_KEY_LENGTH) return FLASH_STORAGE_ERROR_KEY_TOO_LONG;
    if (value.length() > FLASH_STORAGE_MAX_STRING_LENGTH) return FLASH_STORAGE_ERROR_SIZE_TOO_LARGE;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(FLASH_STORAGE_TIMEOUT_MS)) != pdTRUE) {
        return FLASH_STORAGE_ERROR_TIMEOUT;
    }
    
    size_t written = preferences.putString(key, value);
    xSemaphoreGive(mutex);
    
    if (written == 0) {
        return FLASH_STORAGE_ERROR_WRITE_FAILED;
    }
    
    stats.totalWrites++;
    stats.lastWriteTime = millis();
    return FLASH_STORAGE_OK;
}

FlashStorageStatus FlashStorageManager::loadString(const char* key, String& value) {
    if (!initialized) return FLASH_STORAGE_ERROR_NOT_INITIALIZED;
    if (strlen(key) > FLASH_STORAGE_MAX_KEY_LENGTH) return FLASH_STORAGE_ERROR_KEY_TOO_LONG;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(FLASH_STORAGE_TIMEOUT_MS)) != pdTRUE) {
        return FLASH_STORAGE_ERROR_TIMEOUT;
    }
    
    if (!preferences.isKey(key)) {
        xSemaphoreGive(mutex);
        return FLASH_STORAGE_ERROR_KEY_NOT_FOUND;
    }
    
    value = preferences.getString(key, "");
    xSemaphoreGive(mutex);
    
    stats.totalReads++;
    stats.lastReadTime = millis();
    return FLASH_STORAGE_OK;
}

String FlashStorageManager::loadString(const char* key, const String& defaultValue) {
    if (!initialized) return defaultValue;
    if (strlen(key) > FLASH_STORAGE_MAX_KEY_LENGTH) return defaultValue;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(FLASH_STORAGE_TIMEOUT_MS)) != pdTRUE) {
        return defaultValue;
    }
    
    String value = preferences.getString(key, defaultValue);
    xSemaphoreGive(mutex);
    
    stats.totalReads++;
    stats.lastReadTime = millis();
    return value;
}

// ============================================================================
// OPERACIONES CON TIPOS PRIMITIVOS
// ============================================================================

FlashStorageStatus FlashStorageManager::saveInt(const char* key, int32_t value) {
    if (!initialized) return FLASH_STORAGE_ERROR_NOT_INITIALIZED;
    if (strlen(key) > FLASH_STORAGE_MAX_KEY_LENGTH) return FLASH_STORAGE_ERROR_KEY_TOO_LONG;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(FLASH_STORAGE_TIMEOUT_MS)) != pdTRUE) {
        return FLASH_STORAGE_ERROR_TIMEOUT;
    }
    
    size_t written = preferences.putInt(key, value);
    xSemaphoreGive(mutex);
    
    if (written == 0) {
        return FLASH_STORAGE_ERROR_WRITE_FAILED;
    }
    
    stats.totalWrites++;
    stats.lastWriteTime = millis();
    return FLASH_STORAGE_OK;
}

FlashStorageStatus FlashStorageManager::loadInt(const char* key, int32_t& value) {
    if (!initialized) return FLASH_STORAGE_ERROR_NOT_INITIALIZED;
    if (strlen(key) > FLASH_STORAGE_MAX_KEY_LENGTH) return FLASH_STORAGE_ERROR_KEY_TOO_LONG;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(FLASH_STORAGE_TIMEOUT_MS)) != pdTRUE) {
        return FLASH_STORAGE_ERROR_TIMEOUT;
    }
    
    if (!preferences.isKey(key)) {
        xSemaphoreGive(mutex);
        return FLASH_STORAGE_ERROR_KEY_NOT_FOUND;
    }
    
    value = preferences.getInt(key, 0);
    xSemaphoreGive(mutex);
    
    stats.totalReads++;
    stats.lastReadTime = millis();
    return FLASH_STORAGE_OK;
}

int32_t FlashStorageManager::loadInt(const char* key, int32_t defaultValue) {
    if (!initialized) return defaultValue;
    if (strlen(key) > FLASH_STORAGE_MAX_KEY_LENGTH) return defaultValue;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(FLASH_STORAGE_TIMEOUT_MS)) != pdTRUE) {
        return defaultValue;
    }
    
    int32_t value = preferences.getInt(key, defaultValue);
    xSemaphoreGive(mutex);
    
    stats.totalReads++;
    stats.lastReadTime = millis();
    return value;
}

FlashStorageStatus FlashStorageManager::saveUInt(const char* key, uint32_t value) {
    if (!initialized) return FLASH_STORAGE_ERROR_NOT_INITIALIZED;
    if (strlen(key) > FLASH_STORAGE_MAX_KEY_LENGTH) return FLASH_STORAGE_ERROR_KEY_TOO_LONG;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(FLASH_STORAGE_TIMEOUT_MS)) != pdTRUE) {
        return FLASH_STORAGE_ERROR_TIMEOUT;
    }
    
    size_t written = preferences.putUInt(key, value);
    xSemaphoreGive(mutex);
    
    if (written == 0) {
        return FLASH_STORAGE_ERROR_WRITE_FAILED;
    }
    
    stats.totalWrites++;
    stats.lastWriteTime = millis();
    return FLASH_STORAGE_OK;
}

FlashStorageStatus FlashStorageManager::loadUInt(const char* key, uint32_t& value) {
    if (!initialized) return FLASH_STORAGE_ERROR_NOT_INITIALIZED;
    if (strlen(key) > FLASH_STORAGE_MAX_KEY_LENGTH) return FLASH_STORAGE_ERROR_KEY_TOO_LONG;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(FLASH_STORAGE_TIMEOUT_MS)) != pdTRUE) {
        return FLASH_STORAGE_ERROR_TIMEOUT;
    }
    
    if (!preferences.isKey(key)) {
        xSemaphoreGive(mutex);
        return FLASH_STORAGE_ERROR_KEY_NOT_FOUND;
    }
    
    value = preferences.getUInt(key, 0);
    xSemaphoreGive(mutex);
    
    stats.totalReads++;
    stats.lastReadTime = millis();
    return FLASH_STORAGE_OK;
}

uint32_t FlashStorageManager::loadUInt(const char* key, uint32_t defaultValue) {
    if (!initialized) return defaultValue;
    if (strlen(key) > FLASH_STORAGE_MAX_KEY_LENGTH) return defaultValue;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(FLASH_STORAGE_TIMEOUT_MS)) != pdTRUE) {
        return defaultValue;
    }
    
    uint32_t value = preferences.getUInt(key, defaultValue);
    xSemaphoreGive(mutex);
    
    stats.totalReads++;
    stats.lastReadTime = millis();
    return value;
}

FlashStorageStatus FlashStorageManager::saveBool(const char* key, bool value) {
    if (!initialized) return FLASH_STORAGE_ERROR_NOT_INITIALIZED;
    if (strlen(key) > FLASH_STORAGE_MAX_KEY_LENGTH) return FLASH_STORAGE_ERROR_KEY_TOO_LONG;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(FLASH_STORAGE_TIMEOUT_MS)) != pdTRUE) {
        return FLASH_STORAGE_ERROR_TIMEOUT;
    }
    
    size_t written = preferences.putBool(key, value);
    xSemaphoreGive(mutex);
    
    if (written == 0) {
        return FLASH_STORAGE_ERROR_WRITE_FAILED;
    }
    
    stats.totalWrites++;
    stats.lastWriteTime = millis();
    return FLASH_STORAGE_OK;
}

FlashStorageStatus FlashStorageManager::loadBool(const char* key, bool& value) {
    if (!initialized) return FLASH_STORAGE_ERROR_NOT_INITIALIZED;
    if (strlen(key) > FLASH_STORAGE_MAX_KEY_LENGTH) return FLASH_STORAGE_ERROR_KEY_TOO_LONG;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(FLASH_STORAGE_TIMEOUT_MS)) != pdTRUE) {
        return FLASH_STORAGE_ERROR_TIMEOUT;
    }
    
    if (!preferences.isKey(key)) {
        xSemaphoreGive(mutex);
        return FLASH_STORAGE_ERROR_KEY_NOT_FOUND;
    }
    
    value = preferences.getBool(key, false);
    xSemaphoreGive(mutex);
    
    stats.totalReads++;
    stats.lastReadTime = millis();
    return FLASH_STORAGE_OK;
}

bool FlashStorageManager::loadBool(const char* key, bool defaultValue) {
    if (!initialized) return defaultValue;
    if (strlen(key) > FLASH_STORAGE_MAX_KEY_LENGTH) return defaultValue;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(FLASH_STORAGE_TIMEOUT_MS)) != pdTRUE) {
        return defaultValue;
    }
    
    bool value = preferences.getBool(key, defaultValue);
    xSemaphoreGive(mutex);
    
    stats.totalReads++;
    stats.lastReadTime = millis();
    return value;
}

FlashStorageStatus FlashStorageManager::saveFloat(const char* key, float value) {
    if (!initialized) return FLASH_STORAGE_ERROR_NOT_INITIALIZED;
    if (strlen(key) > FLASH_STORAGE_MAX_KEY_LENGTH) return FLASH_STORAGE_ERROR_KEY_TOO_LONG;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(FLASH_STORAGE_TIMEOUT_MS)) != pdTRUE) {
        return FLASH_STORAGE_ERROR_TIMEOUT;
    }
    
    size_t written = preferences.putFloat(key, value);
    xSemaphoreGive(mutex);
    
    if (written == 0) {
        return FLASH_STORAGE_ERROR_WRITE_FAILED;
    }
    
    stats.totalWrites++;
    stats.lastWriteTime = millis();
    return FLASH_STORAGE_OK;
}

FlashStorageStatus FlashStorageManager::loadFloat(const char* key, float& value) {
    if (!initialized) return FLASH_STORAGE_ERROR_NOT_INITIALIZED;
    if (strlen(key) > FLASH_STORAGE_MAX_KEY_LENGTH) return FLASH_STORAGE_ERROR_KEY_TOO_LONG;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(FLASH_STORAGE_TIMEOUT_MS)) != pdTRUE) {
        return FLASH_STORAGE_ERROR_TIMEOUT;
    }
    
    if (!preferences.isKey(key)) {
        xSemaphoreGive(mutex);
        return FLASH_STORAGE_ERROR_KEY_NOT_FOUND;
    }
    
    value = preferences.getFloat(key, 0.0f);
    xSemaphoreGive(mutex);
    
    stats.totalReads++;
    stats.lastReadTime = millis();
    return FLASH_STORAGE_OK;
}

float FlashStorageManager::loadFloat(const char* key, float defaultValue) {
    if (!initialized) return defaultValue;
    if (strlen(key) > FLASH_STORAGE_MAX_KEY_LENGTH) return defaultValue;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(FLASH_STORAGE_TIMEOUT_MS)) != pdTRUE) {
        return defaultValue;
    }
    
    float value = preferences.getFloat(key, defaultValue);
    xSemaphoreGive(mutex);
    
    stats.totalReads++;
    stats.lastReadTime = millis();
    return value;
}

// ============================================================================
// UTILIDADES
// ============================================================================

bool FlashStorageManager::exists(const char* key) {
    if (!initialized) return false;
    if (strlen(key) > FLASH_STORAGE_MAX_KEY_LENGTH) return false;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(FLASH_STORAGE_TIMEOUT_MS)) != pdTRUE) {
        return false;
    }
    
    bool result = preferences.isKey(key);
    xSemaphoreGive(mutex);
    
    return result;
}

FlashStorageStatus FlashStorageManager::remove(const char* key) {
    if (!initialized) return FLASH_STORAGE_ERROR_NOT_INITIALIZED;
    if (strlen(key) > FLASH_STORAGE_MAX_KEY_LENGTH) return FLASH_STORAGE_ERROR_KEY_TOO_LONG;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(FLASH_STORAGE_TIMEOUT_MS)) != pdTRUE) {
        return FLASH_STORAGE_ERROR_TIMEOUT;
    }
    
    bool result = preferences.remove(key);
    xSemaphoreGive(mutex);
    
    return result ? FLASH_STORAGE_OK : FLASH_STORAGE_ERROR_WRITE_FAILED;
}

FlashStorageStatus FlashStorageManager::clear() {
    if (!initialized) return FLASH_STORAGE_ERROR_NOT_INITIALIZED;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(FLASH_STORAGE_TIMEOUT_MS)) != pdTRUE) {
        return FLASH_STORAGE_ERROR_TIMEOUT;
    }
    
    bool result = preferences.clear();
    xSemaphoreGive(mutex);
    
    Serial.println("[FLASH STORAGE] Todas las keys eliminadas");
    
    return result ? FLASH_STORAGE_OK : FLASH_STORAGE_ERROR_WRITE_FAILED;
}

size_t FlashStorageManager::getFreeEntries() {
    if (!initialized) return 0;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(FLASH_STORAGE_TIMEOUT_MS)) != pdTRUE) {
        return 0;
    }
    
    size_t free = preferences.freeEntries();
    xSemaphoreGive(mutex);
    
    return free;
}

// ============================================================================
// ESTADÍSTICAS Y DIAGNÓSTICO
// ============================================================================

void FlashStorageManager::resetStats() {
    memset(&stats, 0, sizeof(FlashStorageStats));
    Serial.println("[FLASH STORAGE] Estadísticas reseteadas");
}

void FlashStorageManager::printStats() {
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║   Flash Storage - Estadísticas         ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.printf("  Namespace: %s\n", namespaceName);
    Serial.printf("  Escrituras totales: %lu\n", stats.totalWrites);
    Serial.printf("  Lecturas totales: %lu\n", stats.totalReads);
    Serial.printf("  Errores CRC: %lu\n", stats.crcErrors);
    Serial.printf("  Errores de versión: %lu\n", stats.versionMismatches);
    Serial.printf("  Última escritura: %lu ms\n", stats.lastWriteTime);
    Serial.printf("  Última lectura: %lu ms\n", stats.lastReadTime);
    Serial.printf("  Entradas libres: %zu\n", getFreeEntries());
    Serial.println("════════════════════════════════════════\n");
}

void FlashStorageManager::printAllKeys() {
    if (!initialized) {
        Serial.println("[FLASH STORAGE] No inicializado");
        return;
    }
    
    Serial.println("\n[FLASH STORAGE] Keys guardadas:");
    Serial.println("────────────────────────────────────────");
    
    // Nota: Preferences no tiene una API para listar todas las keys
    // Esta es una limitación de NVS en ESP32
    Serial.println("  (Preferences/NVS no soporta enumeración)");
    Serial.println("  Usa exists(key) para verificar keys específicas");
    Serial.println("────────────────────────────────────────\n");
}

// ============================================================================
// CRC16
// ============================================================================

uint16_t FlashStorageManager::calculateCRC16(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFF;
    
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;  // Polynomial para Modbus CRC16
            } else {
                crc = crc >> 1;
            }
        }
    }
    
    return crc;
}
