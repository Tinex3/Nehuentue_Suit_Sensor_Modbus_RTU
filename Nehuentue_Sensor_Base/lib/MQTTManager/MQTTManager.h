/**
 * @file MQTTManager.h
 * @brief Manager MQTT con auto-reconnect y FreeRTOS
 * @version 1.0.0
 * @date 2025-10-19
 */

#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#define MQTT_MANAGER_VERSION "1.0.0"
#define MQTT_MANAGER_TASK_STACK_SIZE 4096
#define MQTT_MANAGER_TASK_PRIORITY 3
#define MQTT_MANAGER_RECONNECT_INTERVAL 5000
#define MQTT_MANAGER_KEEP_ALIVE 60
#define MQTT_MANAGER_MAX_PACKET_SIZE 1024
#define MQTT_MANAGER_QUEUE_SIZE 10

// Estructuras
struct MQTTConfig {
    char server[64];
    uint16_t port;
    char user[32];
    char password[64];
    char clientId[32];
    uint16_t keepAlive;
    uint16_t maxPacketSize;
};

struct MQTTMessage {
    char topic[128];
    char payload[512];
    bool retained;
};

struct MQTTStats {
    uint32_t totalPublished;
    uint32_t totalReceived;
    uint32_t failedPublish;
    uint32_t reconnects;
    unsigned long lastPublishTime;
    unsigned long lastReceiveTime;
};

// Callbacks
typedef std::function<void(char*, uint8_t*, unsigned int)> MQTTMessageCallback;
typedef std::function<void(bool)> MQTTConnectionCallback;

class MQTTManager {
public:
    MQTTManager();
    ~MQTTManager();
    
    // Init
    bool begin(const char* server, uint16_t port, const char* user, const char* password, const char* clientId = nullptr);
    void end();
    bool isReady() const { return initialized; }
    
    // Connection
    bool connect();
    void disconnect();
    bool isConnected();
    bool reconnect();
    
    // Publish
    bool publish(const char* topic, const char* payload, bool retained = false);
    bool publish(const char* topic, const uint8_t* payload, size_t length, bool retained = false);
    bool publishJSON(const char* topic, const char* json, bool retained = false);
    
    // Subscribe
    bool subscribe(const char* topic, uint8_t qos = 0);
    bool unsubscribe(const char* topic);
    
    // Callbacks
    void onMessage(MQTTMessageCallback callback);
    void onConnectionChange(MQTTConnectionCallback callback);
    
    // Config
    void setKeepAlive(uint16_t seconds);
    void setMaxPacketSize(uint16_t size);
    void setAutoReconnect(bool enable);
    
    // Info
    MQTTStats getStats() const { return stats; }
    void resetStats();
    void printStats();
    void printInfo();
    
    // Loop (call in main loop if not using FreeRTOS task)
    void loop();
    
private:
    bool initialized;
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    
    TaskHandle_t mqttTaskHandle;
    SemaphoreHandle_t mutex;
    QueueHandle_t publishQueue;
    
    MQTTConfig config;
    MQTTStats stats;
    MQTTMessageCallback messageCallback;
    MQTTConnectionCallback connectionCallback;
    
    bool autoReconnectEnabled;
    unsigned long lastReconnectAttempt;
    
    static void mqttTask(void* parameter);
    static void mqttCallback(char* topic, byte* payload, unsigned int length);
    static MQTTManager* instance;
    
    void lock();
    void unlock();
    void handleReconnect();
    void processPublishQueue();
};

extern MQTTManager MqttMgr;

#endif // MQTT_MANAGER_H
