# 📡 MQTTManager - Gestor MQTT para ESP32

**Versión:** 1.0.0  
**Arquitectura:** FreeRTOS + PubSubClient  
**Thread-Safe:** ✅ Sí (Mutex)

## 📋 Descripción

MQTTManager es una librería profesional para gestión de MQTT en ESP32, con auto-reconexión, cola de publicación con FreeRTOS, callbacks personalizables y estadísticas detalladas.

## ✨ Características

- ✅ **Auto-reconexión**: Reconexión automática configurable
- ✅ **Cola de publicación**: FreeRTOS queue para mensajes offline
- ✅ **Thread-safe**: Operaciones protegidas con mutex
- ✅ **Callbacks**: Eventos de conexión y mensajes
- ✅ **Estadísticas**: Tracking de publicaciones y recepciones
- ✅ **JSON support**: Publicación de payloads JSON
- ✅ **Configurable**: Keep-alive, packet size, QoS

## 📦 Dependencias

```ini
lib_deps = 
    knolleary/PubSubClient@^2.8
```

## 🚀 Uso Rápido

### Ejemplo Básico

```cpp
#include <MQTTManager.h>

void onMQTTMessage(char* topic, byte* payload, unsigned int length) {
    Serial.printf("Mensaje recibido [%s]: ", topic);
    Serial.write(payload, length);
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    
    // Conectar WiFi primero...
    
    // Inicializar MQTT
    MqttMgr.begin("broker.mqtt.com", 1883, "user", "password");
    
    // Callback para mensajes
    MqttMgr.onMessage(onMQTTMessage);
    
    // Conectar
    if (MqttMgr.connect()) {
        MqttMgr.subscribe("sensor/+/command");
        MqttMgr.publish("device/status", "online");
    }
}

void loop() {
    MqttMgr.loop();  // Mantener conexión
    delay(10);
}
```

## 📚 API Completa

### Inicialización

```cpp
// Inicializar con server y credenciales
bool begin(const char* server, uint16_t port = 1883, 
           const char* user = "", const char* password = "",
           const char* clientId = nullptr);

// Finalizar y liberar recursos
void end();
```

### Conexión

```cpp
// Conectar al broker
bool connect();

// Desconectar
void disconnect();

// Verificar conexión
bool isConnected();

// Reconectar manualmente
bool reconnect();
```

### Publicación

```cpp
// Publicar string
bool publish(const char* topic, const char* payload, bool retained = false);

// Publicar binario
bool publish(const char* topic, const uint8_t* payload, size_t length, bool retained = false);

// Publicar JSON
bool publishJSON(const char* topic, const char* json, bool retained = false);
```

### Suscripción

```cpp
// Suscribirse a topic
bool subscribe(const char* topic, uint8_t qos = 0);

// Desuscribirse
bool unsubscribe(const char* topic);
```

### Callbacks

```cpp
// Callback para mensajes recibidos
void onMessage(MQTTMessageCallback callback);
// Firma: void callback(char* topic, byte* payload, unsigned int length)

// Callback para cambios de conexión
void onConnectionChange(MQTTConnectionCallback callback);
// Firma: void callback(bool connected)
```

### Configuración

```cpp
// Keep-alive (segundos)
void setKeepAlive(uint16_t seconds);

// Tamaño máximo de paquete (bytes)
void setMaxPacketSize(uint16_t size);

// Auto-reconexión
void setAutoReconnect(bool enable);
```

### Estadísticas

```cpp
// Obtener estadísticas
const MQTTStats& getStats();

// Reset estadísticas
void resetStats();

// Imprimir estadísticas
void printStats();

// Información del manager
void printInfo();
```

## 📝 Ejemplos Avanzados

### Ejemplo 1: Publicación JSON

```cpp
#include <ArduinoJson.h>
#include <MQTTManager.h>

void publishSensorData() {
    StaticJsonDocument<256> doc;
    doc["sensor"] = "DHT22";
    doc["temperature"] = 23.5;
    doc["humidity"] = 65.3;
    doc["timestamp"] = millis();
    
    char buffer[256];
    serializeJson(doc, buffer);
    
    MqttMgr.publishJSON("sensor/data", buffer);
}
```

### Ejemplo 2: Manejo de Comandos

```cpp
void onCommand(char* topic, byte* payload, unsigned int length) {
    String topicStr = String(topic);
    String payloadStr;
    for (unsigned int i = 0; i < length; i++) {
        payloadStr += (char)payload[i];
    }
    
    if (topicStr.endsWith("/led")) {
        if (payloadStr == "ON") {
            digitalWrite(LED_PIN, HIGH);
            MqttMgr.publish("device/led/state", "ON");
        } else if (payloadStr == "OFF") {
            digitalWrite(LED_PIN, LOW);
            MqttMgr.publish("device/led/state", "OFF");
        }
    }
}

void setup() {
    MqttMgr.begin("broker.mqtt.com", 1883, "user", "pass");
    MqttMgr.onMessage(onCommand);
    MqttMgr.connect();
    MqttMgr.subscribe("device/+/command");
}
```

### Ejemplo 3: Con FreeRTOS Task

```cpp
void mqttTask(void* parameter) {
    while (true) {
        MqttMgr.loop();
        
        // Publicar cada 10 segundos
        static uint32_t lastPublish = 0;
        if (millis() - lastPublish > 10000) {
            MqttMgr.publish("device/heartbeat", "alive");
            lastPublish = millis();
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void setup() {
    MqttMgr.begin("broker.mqtt.com", 1883);
    MqttMgr.connect();
    
    // Crear task FreeRTOS
    xTaskCreatePinnedToCore(
        mqttTask,
        "MQTT Task",
        4096,
        NULL,
        1,
        NULL,
        1
    );
}
```

### Ejemplo 4: Mensajes con Retain

```cpp
void publishDeviceInfo() {
    // Mensaje retained - se mantiene en broker
    MqttMgr.publish("device/info/firmware", "v2.0.0", true);
    MqttMgr.publish("device/info/model", "ESP32-C3", true);
    
    // Estado actual sin retain
    MqttMgr.publish("device/status", "online", false);
}
```

### Ejemplo 5: Callback de Conexión

```cpp
void onConnectionChange(bool connected) {
    if (connected) {
        Serial.println("MQTT conectado - Suscribiendo...");
        MqttMgr.subscribe("device/+/command");
        MqttMgr.publish("device/status", "online", true);
    } else {
        Serial.println("MQTT desconectado");
    }
}

void setup() {
    MqttMgr.begin("broker.mqtt.com", 1883);
    MqttMgr.onConnectionChange(onConnectionChange);
    MqttMgr.connect();
}
```

### Ejemplo 6: Cola de Publicación

```cpp
// Los mensajes se encolan automáticamente si no hay conexión
void loop() {
    // Si está desconectado, los mensajes se encolan
    MqttMgr.publish("sensor/temperature", String(getTemp()).c_str());
    
    // Cuando reconecte, se enviarán automáticamente
    MqttMgr.loop();
    delay(1000);
}
```

## 🔧 Configuración

### Constantes Configurables (MQTTManager.h)

```cpp
#define MQTT_MANAGER_RECONNECT_INTERVAL  5000   // ms entre reconexiones
#define MQTT_MANAGER_QUEUE_SIZE          20     // mensajes en cola
#define MQTT_MANAGER_KEEP_ALIVE          60     // segundos
#define MQTT_MANAGER_MAX_PACKET_SIZE     512    // bytes
```

### Ejemplo de Configuración Personalizada

```cpp
MqttMgr.begin("broker.hivemq.com", 1883);
MqttMgr.setKeepAlive(120);           // 2 minutos
MqttMgr.setMaxPacketSize(1024);      // 1 KB
MqttMgr.setAutoReconnect(true);      // Habilitado por defecto
```

## 📊 Estadísticas

La estructura `MQTTStats` contiene:

```cpp
struct MQTTStats {
    uint32_t totalPublished;     // Total mensajes publicados
    uint32_t totalReceived;      // Total mensajes recibidos
    uint32_t failedPublish;      // Publicaciones fallidas
    uint32_t reconnects;         // Número de reconexiones
    uint32_t lastPublishTime;    // Timestamp última publicación
    uint32_t lastReceiveTime;    // Timestamp última recepción
};
```

**Imprimir estadísticas:**

```cpp
MqttMgr.printStats();
```

## 🛡️ Thread Safety

- ✅ Todas las operaciones protegidas con mutex
- ✅ Safe para uso con múltiples tasks FreeRTOS
- ✅ Cola thread-safe para publicación

## 🔍 Troubleshooting

### Problema: No conecta

```cpp
// Verificar estado de conexión
if (!MqttMgr.isConnected()) {
    Serial.println("No conectado al broker MQTT");
    MqttMgr.printInfo();  // Ver detalles
}
```

### Problema: Mensajes no se reciben

```cpp
// Asegurarse de llamar loop()
void loop() {
    MqttMgr.loop();  // CRÍTICO - mantiene conexión
    delay(10);
}
```

### Problema: Cola llena

```cpp
// Aumentar tamaño de cola
#define MQTT_MANAGER_QUEUE_SIZE 50  // En MQTTManager.h
```

## 📄 Licencia

MIT License - Uso libre con atribución

## 🔗 Referencias

- [PubSubClient](https://github.com/knolleary/pubsubclient)
- [MQTT Protocol](https://mqtt.org/)
- [HiveMQ Public Broker](https://www.hivemq.com/public-mqtt-broker/)

---

**Desarrollado para Nehuentue Suit Sensor v2.0**
