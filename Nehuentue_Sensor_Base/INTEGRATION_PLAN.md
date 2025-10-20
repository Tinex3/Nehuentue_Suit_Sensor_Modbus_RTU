# 🔌 Plan de Integración - Arquitectura Modular

**Objetivo:** Integrar las 5 librerías creadas en el código existente de `main.cpp`, `tasks.cpp` y `web_server.cpp`.

---

## 📋 Checklist de Integración

### ✅ Preparación (COMPLETADO)
- [x] FlashStorageManager creado
- [x] WiFiManager creado
- [x] MQTTManager creado
- [x] ModbusManager creado
- [x] SystemManager creado
- [x] Compilación exitosa

### ⏳ Fase 1: Refactorizar main.cpp

#### Paso 1: Actualizar includes
```cpp
// ANTES (legacy)
#include "eeprom_manager.h"
#include "modbus_rtu.h"
#include "tasks.h"
#include "web_server.h"

// DESPUÉS (con managers)
#include <FlashStorageManager.h>
#include <WiFiManager.h>
#include <MQTTManager.h>
#include <ModbusManager.h>
#include <SystemManager.h>
#include "web_server.h"  // Se refactorizará después
#include "tasks.h"       // Se refactorizará después
```

#### Paso 2: Inicialización en setup()
```cpp
void setup() {
  Serial.begin(115200);
  delay(500);
  
  // 1. System Manager (primero - info del sistema)
  SysMgr.begin();
  SysMgr.printInfo();
  
  // 2. Flash Storage Manager (persistencia)
  if (!FlashStorageMgr.begin("nehuentue")) {
    Serial.println("ERROR: FlashStorageManager falló");
  }
  
  // 3. Cargar configuración desde flash
  WiFiConfig wifiConfig;
  MQTTConfig mqttConfig;
  SensorConfig sensorConfig;
  
  FlashStorageMgr.load("wifi_config", wifiConfig);
  FlashStorageMgr.load("mqtt_config", mqttConfig);
  FlashStorageMgr.load("sensor_config", sensorConfig);
  
  // 4. WiFi Manager (conectividad)
  WiFiMgr.begin("Nehuentue-Sensor");
  WiFiMgr.onEvent(onWiFiEvent);
  
  if (strlen(wifiConfig.ssid) > 0) {
    WiFiMgr.connectSTA(wifiConfig.ssid, wifiConfig.password);
  } else {
    WiFiMgr.startAP("Nehuentue-Config", "12345678");
  }
  
  // 5. MQTT Manager
  if (strlen(mqttConfig.server) > 0) {
    MqttMgr.begin(mqttConfig.server, mqttConfig.port, 
                  mqttConfig.user, mqttConfig.password);
    MqttMgr.onMessage(onMqttMessage);
    MqttMgr.setAutoReconnect(true);
  }
  
  // 6. Modbus Manager
  ModbusMgr.begin(Serial1, sensorConfig.rxPin, sensorConfig.txPin, 
                  sensorConfig.baudrate);
  ModbusMgr.setTimeout(1000);
  
  // 7. Web Server (legacy - por ahora)
  setupWebServer();
  AsyncElegantOTA.begin(&webServer);
  webServer.begin();
  
  // 8. Crear tasks FreeRTOS
  xTaskCreatePinnedToCore(wifiTask, "WiFi Task", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(mqttTask, "MQTT Task", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(modbusTask, "Modbus Task", 4096, NULL, 2, NULL, 1);
  
  Serial.println("\n✅ Sistema inicializado correctamente\n");
  SysMgr.printStatus();
}

void loop() {
  SysMgr.loop();
  delay(10);
}
```

#### Paso 3: Callbacks de WiFi
```cpp
void onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case WIFI_EVENT_STA_CONNECTED:
      Serial.println("[WiFi] Conectado a la red");
      break;
      
    case WIFI_EVENT_STA_DISCONNECTED:
      Serial.println("[WiFi] Desconectado");
      break;
      
    case WIFI_EVENT_STA_GOT_IP:
      Serial.printf("[WiFi] IP obtenida: %s\n", WiFi.localIP().toString().c_str());
      // Conectar MQTT ahora que tenemos IP
      if (MqttMgr.isInitialized() && !MqttMgr.isConnected()) {
        MqttMgr.connect();
      }
      break;
  }
}
```

#### Paso 4: Callback de MQTT
```cpp
void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String topicStr = String(topic);
  String payloadStr;
  
  for (unsigned int i = 0; i < length; i++) {
    payloadStr += (char)payload[i];
  }
  
  Serial.printf("[MQTT] Mensaje recibido [%s]: %s\n", topic, payloadStr.c_str());
  
  // Procesar comandos
  if (topicStr.endsWith("/command")) {
    handleMqttCommand(payloadStr);
  }
}

void handleMqttCommand(const String& command) {
  if (command == "restart") {
    Serial.println("[CMD] Reiniciando...");
    SysMgr.restart(1000);
  } else if (command == "status") {
    SysMgr.printStatus();
    ModbusMgr.printStats();
    MqttMgr.printStats();
  }
}
```

---

### ⏳ Fase 2: Refactorizar tasks.cpp

#### Task WiFi (simplificada)
```cpp
void wifiTask(void* parameter) {
  while (true) {
    WiFiMgr.loop();  // Manejo de reconexión automática
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
```

#### Task MQTT (simplificada)
```cpp
void mqttTask(void* parameter) {
  while (true) {
    if (WiFiMgr.isConnected()) {
      MqttMgr.loop();  // Mantiene conexión y procesa cola
      
      // Publicar heartbeat cada 30 segundos
      static uint32_t lastHeartbeat = 0;
      if (millis() - lastHeartbeat > 30000) {
        MqttMgr.publish("nehuentue/heartbeat", "alive");
        lastHeartbeat = millis();
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
```

#### Task Modbus (refactorizada)
```cpp
void modbusTask(void* parameter) {
  SensorConfig config;
  FlashStorageMgr.load("sensor_config", config);
  
  while (true) {
    if (config.enabled) {
      // Leer registros del sensor
      ModbusResponse resp = ModbusMgr.readHoldingRegisters(
        config.slaveId, 
        config.startAddress, 
        config.quantity
      );
      
      if (resp.success) {
        // Extraer datos
        uint16_t registers[125];
        uint16_t count = ModbusMgr.extractRegisters(resp, registers, 125);
        
        // Publicar a MQTT si está conectado
        if (MqttMgr.isConnected()) {
          publishSensorData(registers, count);
        }
        
        stats.successfulReads++;
      } else {
        stats.failedReads++;
        Serial.printf("[Modbus] Error: %d\n", resp.exceptionCode);
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(config.pollInterval));
  }
}

void publishSensorData(uint16_t* registers, uint16_t count) {
  StaticJsonDocument<512> doc;
  JsonArray data = doc.createNestedArray("registers");
  
  for (uint16_t i = 0; i < count; i++) {
    data.add(registers[i]);
  }
  
  doc["timestamp"] = millis();
  doc["count"] = count;
  
  char buffer[512];
  serializeJson(doc, buffer);
  
  MqttMgr.publishJSON("nehuentue/sensor/data", buffer);
}
```

---

### ⏳ Fase 3: Refactorizar web_server.cpp

#### Actualizar endpoint /api/status
```cpp
void handleStatus(AsyncWebServerRequest *request) {
  StaticJsonDocument<1024> doc;
  
  // System info
  SystemStatus sysStatus = SysMgr.getStatus();
  doc["uptime"] = sysStatus.uptime;
  doc["freeHeap"] = sysStatus.freeHeap;
  doc["firmware"] = sysStatus.firmwareVersion;
  
  // WiFi info
  doc["wifi"]["connected"] = WiFiMgr.isConnected();
  if (WiFiMgr.isConnected()) {
    doc["wifi"]["ssid"] = WiFi.SSID();
    doc["wifi"]["ip"] = WiFi.localIP().toString();
    doc["wifi"]["rssi"] = WiFiMgr.getRSSI();
  }
  
  // MQTT info
  doc["mqtt"]["connected"] = MqttMgr.isConnected();
  const MQTTStats& mqttStats = MqttMgr.getStats();
  doc["mqtt"]["published"] = mqttStats.totalPublished;
  doc["mqtt"]["received"] = mqttStats.totalReceived;
  
  // Modbus info
  const ModbusStats& modbusStats = ModbusMgr.getStats();
  doc["modbus"]["totalRequests"] = modbusStats.totalRequests;
  doc["modbus"]["successRate"] = (float)modbusStats.successfulRequests / modbusStats.totalRequests * 100;
  
  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
}
```

#### Actualizar endpoint /api/wifi/save
```cpp
void handleWiFiSave(AsyncWebServerRequest *request) {
  if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
    String ssid = request->getParam("ssid", true)->value();
    String password = request->getParam("password", true)->value();
    
    // Guardar en flash
    WiFiConfig config;
    strncpy(config.ssid, ssid.c_str(), sizeof(config.ssid) - 1);
    strncpy(config.password, password.c_str(), sizeof(config.password) - 1);
    
    if (FlashStorageMgr.save("wifi_config", config)) {
      // Conectar con nueva configuración
      WiFiMgr.connectSTA(config.ssid, config.password);
      
      request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"WiFi configurado\"}");
    } else {
      request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Error al guardar\"}");
    }
  } else {
    request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Parámetros faltantes\"}");
  }
}
```

#### Actualizar endpoint /api/mqtt/save
```cpp
void handleMQTTSave(AsyncWebServerRequest *request) {
  MQTTConfig config;
  
  if (request->hasParam("server", true)) {
    strncpy(config.server, request->getParam("server", true)->value().c_str(), 
            sizeof(config.server) - 1);
  }
  
  if (request->hasParam("port", true)) {
    config.port = request->getParam("port", true)->value().toInt();
  }
  
  if (request->hasParam("user", true)) {
    strncpy(config.user, request->getParam("user", true)->value().c_str(), 
            sizeof(config.user) - 1);
  }
  
  if (request->hasParam("password", true)) {
    strncpy(config.password, request->getParam("password", true)->value().c_str(), 
            sizeof(config.password) - 1);
  }
  
  if (FlashStorageMgr.save("mqtt_config", config)) {
    // Reconectar MQTT con nueva configuración
    MqttMgr.disconnect();
    MqttMgr.begin(config.server, config.port, config.user, config.password);
    MqttMgr.connect();
    
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    request->send(500, "application/json", "{\"status\":\"error\"}");
  }
}
```

---

### ⏳ Fase 4: Crear headers de configuración

#### config.h (nuevo archivo)
```cpp
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Versión de configuración (para migración)
#define CONFIG_VERSION 1

// WiFi Configuration
struct WiFiConfig {
  char ssid[32];
  char password[64];
  bool dhcp;
  char staticIP[16];
  char gateway[16];
  char subnet[16];
  uint8_t version;
  
  WiFiConfig() {
    memset(this, 0, sizeof(WiFiConfig));
    version = CONFIG_VERSION;
    dhcp = true;
  }
};

// MQTT Configuration
struct MQTTConfig {
  char server[64];
  uint16_t port;
  char user[32];
  char password[64];
  char clientId[32];
  uint8_t version;
  
  MQTTConfig() {
    memset(this, 0, sizeof(MQTTConfig));
    version = CONFIG_VERSION;
    port = 1883;
  }
};

// Sensor Configuration
struct SensorConfig {
  bool enabled;
  uint8_t slaveId;
  uint16_t startAddress;
  uint16_t quantity;
  uint32_t pollInterval;
  int rxPin;
  int txPin;
  uint32_t baudrate;
  char name[32];
  uint8_t version;
  
  SensorConfig() {
    memset(this, 0, sizeof(SensorConfig));
    version = CONFIG_VERSION;
    enabled = false;
    pollInterval = 1000;
    baudrate = 9600;
  }
};

// Statistics
struct SystemStats {
  uint32_t successfulReads;
  uint32_t failedReads;
  uint32_t mqttPublished;
  uint32_t wifiReconnects;
};

#endif // CONFIG_H
```

---

## 🔄 Secuencia de Integración Recomendada

### 1️⃣ Primera Iteración (Testing básico)
- [ ] Actualizar main.cpp con managers
- [ ] Compilar y verificar
- [ ] Subir a ESP32
- [ ] Probar inicialización
- [ ] Verificar logs seriales

### 2️⃣ Segunda Iteración (WiFi + Flash)
- [ ] Integrar WiFiManager en tasks
- [ ] Integrar FlashStorageManager
- [ ] Probar guardado/carga de configuración
- [ ] Probar modo AP
- [ ] Probar conexión STA

### 3️⃣ Tercera Iteración (MQTT)
- [ ] Integrar MQTTManager
- [ ] Probar conexión a broker
- [ ] Probar publicación
- [ ] Probar suscripción
- [ ] Probar auto-reconexión

### 4️⃣ Cuarta Iteración (Modbus)
- [ ] Integrar ModbusManager
- [ ] Probar lectura de registros
- [ ] Probar escritura
- [ ] Verificar estadísticas

### 5️⃣ Quinta Iteración (WebServer)
- [ ] Actualizar endpoints con managers
- [ ] Probar guardado desde web
- [ ] Verificar JSON responses
- [ ] Probar OTA

---

## ✅ Validación Final

### Checklist de Testing
- [ ] ✅ Compilación sin errores
- [ ] ✅ Booteo exitoso
- [ ] ✅ Logs coherentes
- [ ] ✅ WiFi AP funciona
- [ ] ✅ Configuración WiFi desde web
- [ ] ✅ Conexión WiFi STA
- [ ] ✅ Configuración MQTT desde web
- [ ] ✅ Conexión MQTT al broker
- [ ] ✅ Publicación MQTT
- [ ] ✅ Configuración sensor desde web
- [ ] ✅ Lectura Modbus funciona
- [ ] ✅ Datos persisten en flash
- [ ] ✅ Reinicio mantiene config
- [ ] ✅ Factory reset funciona
- [ ] ✅ OTA funciona
- [ ] ✅ Estadísticas correctas
- [ ] ✅ Memoria estable

---

## 📊 Métricas Esperadas

### Uso de Memoria
- **RAM:** ~12-15% (40-50 KB)
- **Flash:** ~75-80% (980-1050 KB)

### Performance
- **Boot time:** < 5 segundos
- **WiFi connect:** < 10 segundos
- **MQTT connect:** < 3 segundos
- **Modbus poll:** ~100-500 ms

---

**Última actualización:** 19/10/2025  
**Estado:** LISTO PARA INTEGRACIÓN
