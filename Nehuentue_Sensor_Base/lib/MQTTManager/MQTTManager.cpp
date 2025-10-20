/**
 * @file MQTTManager.cpp
 * @brief Implementación del MQTTManager
 * @version 1.0.0
 * @date 2025-10-19
 */

#include "MQTTManager.h"
#include <WiFi.h>

// Instancia global
MQTTManager MqttMgr;
MQTTManager* MQTTManager::instance = nullptr;

// ============================================================================
// CONSTRUCTOR Y DESTRUCTOR
// ============================================================================

MQTTManager::MQTTManager() : mqttClient(wifiClient) {
    initialized = false;
    mqttTaskHandle = NULL;
    mutex = NULL;
    publishQueue = NULL;
    autoReconnectEnabled = true;
    lastReconnectAttempt = 0;
    messageCallback = nullptr;
    connectionCallback = nullptr;
    
    memset(&config, 0, sizeof(MQTTConfig));
    memset(&stats, 0, sizeof(MQTTStats));
    
    instance = this;
}

MQTTManager::~MQTTManager() {
    end();
}

// ============================================================================
// INICIALIZACIÓN
// ============================================================================

bool MQTTManager::begin(const char* server, uint16_t port, const char* user, const char* password, const char* clientId) {
    if (initialized) {
        Serial.println("[MQTT MGR] Ya inicializado");
        return true;
    }
    
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║   MQTT Manager v1.0                    ║");
    Serial.println("╚════════════════════════════════════════╝");
    
    // Crear mutex
    mutex = xSemaphoreCreateMutex();
    if (mutex == NULL) {
        Serial.println("[MQTT MGR] ERROR: No se pudo crear mutex");
        return false;
    }
    
    // Crear cola de publicación
    publishQueue = xQueueCreate(MQTT_MANAGER_QUEUE_SIZE, sizeof(MQTTMessage));
    if (publishQueue == NULL) {
        Serial.println("[MQTT MGR] ERROR: No se pudo crear cola");
        vSemaphoreDelete(mutex);
        return false;
    }
    
    // Guardar configuración
    strncpy(config.server, server, sizeof(config.server) - 1);
    config.port = port;
    strncpy(config.user, user, sizeof(config.user) - 1);
    strncpy(config.password, password, sizeof(config.password) - 1);
    
    // Client ID
    if (clientId != nullptr) {
        strncpy(config.clientId, clientId, sizeof(config.clientId) - 1);
    } else {
        // Generar client ID único
        uint8_t mac[6];
        WiFi.macAddress(mac);
        snprintf(config.clientId, sizeof(config.clientId), "ESP32-%02X%02X%02X", mac[3], mac[4], mac[5]);
    }
    
    config.keepAlive = MQTT_MANAGER_KEEP_ALIVE;
    config.maxPacketSize = MQTT_MANAGER_MAX_PACKET_SIZE;
    
    // Configurar cliente MQTT
    mqttClient.setServer(server, port);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setKeepAlive(config.keepAlive);
    mqttClient.setBufferSize(config.maxPacketSize);
    
    initialized = true;
    
    Serial.printf("  Server: %s:%d\n", server, port);
    Serial.printf("  Client ID: %s\n", config.clientId);
    Serial.printf("  User: %s\n", user);
    Serial.printf("  Keep Alive: %d s\n", config.keepAlive);
    Serial.printf("  Max Packet: %d bytes\n", config.maxPacketSize);
    Serial.println("════════════════════════════════════════\n");
    
    return true;
}

void MQTTManager::end() {
    if (initialized) {
        disconnect();
        initialized = false;
        Serial.println("[MQTT MGR] Finalizado");
    }
    
    if (mutex != NULL) {
        vSemaphoreDelete(mutex);
        mutex = NULL;
    }
    
    if (publishQueue != NULL) {
        vQueueDelete(publishQueue);
        publishQueue = NULL;
    }
}

// ============================================================================
// CONEXIÓN
// ============================================================================

bool MQTTManager::connect() {
    if (!initialized) {
        Serial.println("[MQTT MGR] ERROR: No inicializado");
        return false;
    }
    
    Serial.println("[MQTT MGR] Conectando al broker MQTT...");
    Serial.printf("  Server: %s:%d\n", config.server, config.port);
    Serial.printf("  Client ID: %s\n", config.clientId);
    
    bool connected = false;
    
    if (strlen(config.user) > 0) {
        connected = mqttClient.connect(config.clientId, config.user, config.password);
    } else {
        connected = mqttClient.connect(config.clientId);
    }
    
    if (connected) {
        Serial.println("[MQTT MGR] ✓ MQTT conectado");
        stats.reconnects++;
        
        if (connectionCallback != nullptr) {
            connectionCallback(true);
        }
    } else {
        Serial.printf("[MQTT MGR] ✗ Conexión fallida (estado: %d)\n", mqttClient.state());
        
        if (connectionCallback != nullptr) {
            connectionCallback(false);
        }
    }
    
    return connected;
}

void MQTTManager::disconnect() {
    if (mqttClient.connected()) {
        mqttClient.disconnect();
        Serial.println("[MQTT MGR] Desconectado");
    }
}

bool MQTTManager::isConnected() {
    return mqttClient.connected();
}

bool MQTTManager::reconnect() {
    if (isConnected()) {
        return true;
    }
    
    unsigned long now = millis();
    if (now - lastReconnectAttempt > MQTT_MANAGER_RECONNECT_INTERVAL) {
        lastReconnectAttempt = now;
        Serial.println("[MQTT MGR] Intentando reconectar...");
        return connect();
    }
    
    return false;
}

// ============================================================================
// PUBLISH
// ============================================================================

bool MQTTManager::publish(const char* topic, const char* payload, bool retained) {
    if (!initialized) return false;
    
    // Si está conectado, publicar directamente
    if (isConnected()) {
        lock();
        bool result = mqttClient.publish(topic, payload, retained);
        unlock();
        
        if (result) {
            stats.totalPublished++;
            stats.lastPublishTime = millis();
        } else {
            stats.failedPublish++;
        }
        
        return result;
    }
    
    // Si no está conectado, encolar mensaje
    MQTTMessage msg;
    strncpy(msg.topic, topic, sizeof(msg.topic) - 1);
    strncpy(msg.payload, payload, sizeof(msg.payload) - 1);
    msg.retained = retained;
    
    if (xQueueSend(publishQueue, &msg, 0) == pdTRUE) {
        Serial.println("[MQTT MGR] Mensaje encolado");
        return true;
    } else {
        Serial.println("[MQTT MGR] Cola llena, mensaje descartado");
        stats.failedPublish++;
        return false;
    }
}

bool MQTTManager::publish(const char* topic, const uint8_t* payload, size_t length, bool retained) {
    if (!initialized || !isConnected()) return false;
    
    lock();
    bool result = mqttClient.publish(topic, payload, length, retained);
    unlock();
    
    if (result) {
        stats.totalPublished++;
        stats.lastPublishTime = millis();
    } else {
        stats.failedPublish++;
    }
    
    return result;
}

bool MQTTManager::publishJSON(const char* topic, const char* json, bool retained) {
    return publish(topic, json, retained);
}

// ============================================================================
// SUBSCRIBE
// ============================================================================

bool MQTTManager::subscribe(const char* topic, uint8_t qos) {
    if (!initialized || !isConnected()) return false;
    
    lock();
    bool result = mqttClient.subscribe(topic, qos);
    unlock();
    
    if (result) {
        Serial.printf("[MQTT MGR] ✓ Suscrito a: %s\n", topic);
    } else {
        Serial.printf("[MQTT MGR] ✗ Error al suscribir: %s\n", topic);
    }
    
    return result;
}

bool MQTTManager::unsubscribe(const char* topic) {
    if (!initialized || !isConnected()) return false;
    
    lock();
    bool result = mqttClient.unsubscribe(topic);
    unlock();
    
    if (result) {
        Serial.printf("[MQTT MGR] Desuscrito de: %s\n", topic);
    }
    
    return result;
}

// ============================================================================
// CALLBACKS
// ============================================================================

void MQTTManager::onMessage(MQTTMessageCallback callback) {
    lock();
    messageCallback = callback;
    unlock();
}

void MQTTManager::onConnectionChange(MQTTConnectionCallback callback) {
    lock();
    connectionCallback = callback;
    unlock();
}

// ============================================================================
// CONFIGURACIÓN
// ============================================================================

void MQTTManager::setKeepAlive(uint16_t seconds) {
    lock();
    config.keepAlive = seconds;
    mqttClient.setKeepAlive(seconds);
    unlock();
}

void MQTTManager::setMaxPacketSize(uint16_t size) {
    lock();
    config.maxPacketSize = size;
    mqttClient.setBufferSize(size);
    unlock();
}

void MQTTManager::setAutoReconnect(bool enable) {
    lock();
    autoReconnectEnabled = enable;
    unlock();
}

// ============================================================================
// INFORMACIÓN
// ============================================================================

void MQTTManager::resetStats() {
    lock();
    memset(&stats, 0, sizeof(MQTTStats));
    unlock();
}

void MQTTManager::printStats() {
    lock();
    
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║   MQTT Manager - Estadísticas          ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.printf("  Mensajes publicados: %lu\n", stats.totalPublished);
    Serial.printf("  Mensajes recibidos: %lu\n", stats.totalReceived);
    Serial.printf("  Publicaciones fallidas: %lu\n", stats.failedPublish);
    Serial.printf("  Reconexiones: %lu\n", stats.reconnects);
    Serial.printf("  Última publicación: %lu ms\n", stats.lastPublishTime);
    Serial.printf("  Última recepción: %lu ms\n", stats.lastReceiveTime);
    Serial.println("════════════════════════════════════════\n");
    
    unlock();
}

void MQTTManager::printInfo() {
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║   MQTT Manager - Información           ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.printf("  Server: %s:%d\n", config.server, config.port);
    Serial.printf("  Client ID: %s\n", config.clientId);
    Serial.printf("  User: %s\n", config.user);
    Serial.printf("  Conectado: %s\n", isConnected() ? "Sí" : "No");
    Serial.printf("  Auto-reconnect: %s\n", autoReconnectEnabled ? "Sí" : "No");
    Serial.printf("  Estado: %d\n", mqttClient.state());
    Serial.println("════════════════════════════════════════\n");
}

// ============================================================================
// LOOP
// ============================================================================

void MQTTManager::loop() {
    if (!initialized) return;
    
    // Mantener conexión
    mqttClient.loop();
    
    // Auto-reconnect si está habilitado
    if (autoReconnectEnabled && !isConnected()) {
        reconnect();
    }
    
    // Procesar cola de publicación
    if (isConnected()) {
        processPublishQueue();
    }
}

// ============================================================================
// MÉTODOS PRIVADOS
// ============================================================================

void MQTTManager::lock() {
    if (mutex != NULL) {
        xSemaphoreTake(mutex, portMAX_DELAY);
    }
}

void MQTTManager::unlock() {
    if (mutex != NULL) {
        xSemaphoreGive(mutex);
    }
}

void MQTTManager::handleReconnect() {
    if (!autoReconnectEnabled || isConnected()) return;
    
    unsigned long now = millis();
    if (now - lastReconnectAttempt > MQTT_MANAGER_RECONNECT_INTERVAL) {
        lastReconnectAttempt = now;
        reconnect();
    }
}

void MQTTManager::processPublishQueue() {
    MQTTMessage msg;
    
    // Procesar hasta 5 mensajes por iteración
    for (int i = 0; i < 5; i++) {
        if (xQueueReceive(publishQueue, &msg, 0) == pdTRUE) {
            publish(msg.topic, msg.payload, msg.retained);
        } else {
            break;  // Cola vacía
        }
    }
}

void MQTTManager::mqttCallback(char* topic, byte* payload, unsigned int length) {
    if (instance == nullptr) return;
    
    instance->stats.totalReceived++;
    instance->stats.lastReceiveTime = millis();
    
    if (instance->messageCallback != nullptr) {
        instance->messageCallback(topic, payload, length);
    }
}

void MQTTManager::mqttTask(void* parameter) {
    MQTTManager* mgr = (MQTTManager*)parameter;
    
    while (true) {
        mgr->loop();
        vTaskDelay(pdMS_TO_TICKS(10));  // 10ms
    }
}
