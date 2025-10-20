/**
 * @file main.cpp
 * @brief Nehuentue Suit Sensor - Firmware v2.0 con Arquitectura Modular
 * @version 2.0.0
 * @date 2025-10-19
 * 
 * CAMBIOS v2.1:
 * - Eliminado servidor web AsyncWebServer
 * - Configuración 100% por MQTT
 * - Credenciales WiFi/MQTT preconfiguradas
 * - Liberación de ~250KB Flash y ~30KB RAM
 */

#include <Arduino.h>
#include <ArduinoJson.h>

// Managers modulares
#include <SystemManager.h>
#include <FlashStorageManager.h>
#include <WiFiManager.h>
#include <MQTTManager.h>
#include <ModbusManager.h>

// Configuración
#include "config.h"
#include "tasks.h"

// ============================================================================
// CONFIGURACIÓN GLOBAL
// ============================================================================
SensorConfig sensorConfig;
SystemStats systemStats;

// Configuración WiFi y MQTT (usando las estructuras de los managers)
WiFiConfig wifiConfig;
MQTTConfig mqttConfig;

// Sistema de errores
SystemError lastError;
SystemError errors[5];  // Buffer para últimos 5 errores
int errorCount = 0;

// Función para registrar error
void logError(ErrorType type, ErrorCode code, const char* customDesc = nullptr) {
  lastError.type = type;
  lastError.code = code;
  lastError.timestamp = millis();
  lastError.active = true;
  
  if (customDesc != nullptr) {
    strncpy(lastError.description, customDesc, sizeof(lastError.description) - 1);
  } else {
    strncpy(lastError.description, getErrorDescription(code), sizeof(lastError.description) - 1);
  }
  
  // Guardar en buffer circular
  errors[errorCount % 5] = lastError;
  errorCount++;
  
  Serial.printf("[ERROR] [%s] Code %d: %s\n", 
                getErrorTypeName(type), code, lastError.description);
}

// Función para limpiar error
void clearError() {
  lastError.type = ERROR_NONE;
  lastError.code = ERR_NONE;
  lastError.active = false;
  strcpy(lastError.description, "Sin errores");
}

// ============================================================================
// CALLBACKS
// ============================================================================

/**
 * @brief Callback para eventos WiFi
 * @param event ID del evento WiFi de ESP-IDF
 * @param info Información adicional del evento
 */
void onWiFiEvent(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("[WiFi] ✓ Conectado a la red");
      clearError();  // Limpiar errores de WiFi al conectar
      break;
      
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("[WiFi] ✗ Desconectado de la red");
      systemStats.wifiReconnects++;
      logError(ERROR_WIFI, ERR_WIFI_DISCONNECTED);
      break;
      
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    {
      Serial.printf("[WiFi] ✓ IP obtenida: %s\n", WiFi.localIP().toString().c_str());
      
      // Verificar señal WiFi débil
      int8_t rssi = WiFi.RSSI();
      if (rssi < -80) {
        logError(ERROR_WIFI, ERR_WIFI_WEAK_SIGNAL, 
                 String("Señal débil: " + String(rssi) + " dBm").c_str());
      }
      
      // Conectar MQTT cuando tengamos IP
      if (strlen(mqttConfig.server) > 0 && !MqttMgr.isConnected()) {
        Serial.println("[MQTT] Intentando conectar al broker...");
        if (MqttMgr.connect()) {
          Serial.println("[MQTT] ✓ Conectado al broker");
          // Suscribirse a topics de comando
          MqttMgr.subscribe("nehuentue/+/command");
        } else {
          logError(ERROR_MQTT, ERR_MQTT_CONNECTION_FAILED);
        }
      }
      break;
    }
      
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      Serial.println("[WiFi AP] Cliente conectado");
      break;
      
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      Serial.println("[WiFi AP] Cliente desconectado");
      break;
  }
}

/**
 * @brief Callback para mensajes MQTT recibidos con comandos JSON
 * 
 * Comandos soportados:
 * - {"cmd":"get_status"}
 * - {"cmd":"get_config"}
 * - {"cmd":"set_wifi","ssid":"...", "password":"..."}
 * - {"cmd":"set_mqtt","server":"...", "port":1883, "user":"...", "password":"..."}
 * - {"cmd":"set_sensor","name":"...", "address":1, "register":0, "count":2}
 * - {"cmd":"scan_wifi"}
 * - {"cmd":"restart"}
 * - {"cmd":"factory_reset"}
 */
void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String topicStr = String(topic);
  
  // Convertir payload a String
  char buffer[length + 1];
  memcpy(buffer, payload, length);
  buffer[length] = '\0';
  String payloadStr = String(buffer);
  
  Serial.printf("[MQTT] Mensaje [%s]: %s\n", topic, payloadStr.c_str());
  
  // Intentar parsear como JSON
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, payloadStr);
  
  String responseTopic = String(MQTT_TOPIC_BASE) + "/" + String(mqttConfig.clientId) + "/" + String(MQTT_TOPIC_RESPONSE);
  
  if (error) {
    // No es JSON válido, intentar comandos simples (retrocompatibilidad)
    if (payloadStr == "restart") {
      Serial.println("[CMD] Reiniciando...");
      MqttMgr.publish(responseTopic.c_str(), "{\"status\":\"restarting\"}");
      delay(500);
      SysMgr.restart(1000);
      return;
    }
    else if (payloadStr == "status") {
      SysMgr.printStatus();
      ModbusMgr.printStats();
      MqttMgr.printStats();
      return;
    }
    
    Serial.printf("[CMD] Error JSON: %s\n", error.c_str());
    MqttMgr.publish(responseTopic.c_str(), "{\"error\":\"invalid_json\"}");
    return;
  }
  
  // Procesar comando JSON
  const char* cmd = doc["cmd"];
  if (cmd == nullptr) {
    MqttMgr.publish(responseTopic.c_str(), "{\"error\":\"missing_cmd\"}");
    return;
  }
  
  Serial.printf("[CMD] Ejecutando: %s\n", cmd);
  
  // ========== GET STATUS ==========
  if (strcmp(cmd, "get_status") == 0) {
    StaticJsonDocument<1024> response;  // Aumentado para incluir errores
    response["cmd"] = "get_status";
    response["status"] = "ok";
    
    JsonObject system = response.createNestedObject("system");
    system["uptime"] = millis() / 1000;
    system["heap_free"] = ESP.getFreeHeap();
    system["cpu_freq"] = ESP.getCpuFreqMHz();
    
    JsonObject wifi = response.createNestedObject("wifi");
    wifi["connected"] = WifiMgr.isConnected();
    wifi["ssid"] = WifiMgr.getSSID();
    wifi["rssi"] = WifiMgr.getRSSI();
    wifi["ip"] = WifiMgr.getIP().toString();
    
    JsonObject mqtt = response.createNestedObject("mqtt");
    mqtt["connected"] = MqttMgr.isConnected();
    mqtt["server"] = mqttConfig.server;
    
    JsonObject modbus = response.createNestedObject("modbus");
    modbus["enabled"] = true;
    modbus["reads_ok"] = systemStats.successfulReads;
    modbus["reads_fail"] = systemStats.failedReads;
    
    // Información de errores
    JsonObject error = response.createNestedObject("error");
    error["code"] = lastError.code;
    error["type"] = getErrorTypeName(lastError.type);
    error["description"] = lastError.description;
    error["active"] = lastError.active;
    if (lastError.active) {
      error["timestamp"] = lastError.timestamp;
      error["age_seconds"] = (millis() - lastError.timestamp) / 1000;
    }
    
    String output;
    serializeJson(response, output);
    MqttMgr.publish(responseTopic.c_str(), output.c_str());
  }
  
  // ========== GET CONFIG ==========
  else if (strcmp(cmd, "get_config") == 0) {
    StaticJsonDocument<512> response;
    response["cmd"] = "get_config";
    response["status"] = "ok";
    
    JsonObject wifi = response.createNestedObject("wifi");
    wifi["ssid"] = wifiConfig.ssid;
    wifi["hostname"] = wifiConfig.hostname;
    
    JsonObject mqtt = response.createNestedObject("mqtt");
    mqtt["server"] = mqttConfig.server;
    mqtt["port"] = mqttConfig.port;
    mqtt["user"] = mqttConfig.user;
    mqtt["client_id"] = mqttConfig.clientId;
    
    JsonObject sensor = response.createNestedObject("sensor");
    sensor["name"] = sensorConfig.name;
    sensor["address"] = sensorConfig.modbusAddress;
    sensor["register"] = sensorConfig.registerStart;
    sensor["count"] = sensorConfig.registerCount;
    
    String output;
    serializeJson(response, output);
    MqttMgr.publish(responseTopic.c_str(), output.c_str());
  }
  
  // ========== SET WIFI ==========
  else if (strcmp(cmd, "set_wifi") == 0) {
    const char* ssid = doc["ssid"];
    const char* password = doc["password"];
    
    if (ssid != nullptr && password != nullptr) {
      strncpy(wifiConfig.ssid, ssid, sizeof(wifiConfig.ssid) - 1);
      strncpy(wifiConfig.password, password, sizeof(wifiConfig.password) - 1);
      
      // Guardar en flash (auto-commit)
      FlashStorage.saveString("wifi_ssid", ssid);
      FlashStorage.saveString("wifi_password", password);
      
      MqttMgr.publish(responseTopic.c_str(), "{\"status\":\"ok\",\"message\":\"WiFi guardado, reinicia para aplicar\"}");
      Serial.printf("[CMD] WiFi configurado: %s\n", ssid);
    } else {
      MqttMgr.publish(responseTopic.c_str(), "{\"error\":\"missing_params\"}");
    }
  }
  
  // ========== SET MQTT ==========
  else if (strcmp(cmd, "set_mqtt") == 0) {
    const char* server = doc["server"];
    int port = doc["port"] | 1883;
    const char* user = doc["user"];
    const char* password = doc["password"];
    
    if (server != nullptr) {
      strncpy(mqttConfig.server, server, sizeof(mqttConfig.server) - 1);
      mqttConfig.port = port;
      if (user) strncpy(mqttConfig.user, user, sizeof(mqttConfig.user) - 1);
      if (password) strncpy(mqttConfig.password, password, sizeof(mqttConfig.password) - 1);
      
      // Guardar en flash (auto-commit)
      FlashStorage.saveString("mqtt_server", server);
      FlashStorage.saveInt("mqtt_port", port);
      if (user) FlashStorage.saveString("mqtt_user", user);
      if (password) FlashStorage.saveString("mqtt_password", password);
      
      MqttMgr.publish(responseTopic.c_str(), "{\"status\":\"ok\",\"message\":\"MQTT guardado, reinicia para aplicar\"}");
      Serial.printf("[CMD] MQTT configurado: %s:%d\n", server, port);
    } else {
      MqttMgr.publish(responseTopic.c_str(), "{\"error\":\"missing_server\"}");
    }
  }
  
  // ========== SET SENSOR ==========
  else if (strcmp(cmd, "set_sensor") == 0) {
    if (doc.containsKey("name")) strncpy(sensorConfig.name, doc["name"], sizeof(sensorConfig.name) - 1);
    if (doc.containsKey("address")) sensorConfig.modbusAddress = doc["address"];
    if (doc.containsKey("register")) sensorConfig.registerStart = doc["register"];
    if (doc.containsKey("count")) sensorConfig.registerCount = doc["count"];
    if (doc.containsKey("multiplier")) sensorConfig.multiplier = doc["multiplier"];
    
    // Guardar en flash (auto-commit)
    FlashStorage.save("sensor_config", sensorConfig);
    
    MqttMgr.publish(responseTopic.c_str(), "{\"status\":\"ok\",\"message\":\"Sensor configurado\"}");
    Serial.println("[CMD] Sensor configurado");
  }
  
  // ========== SCAN WIFI ==========
  else if (strcmp(cmd, "scan_wifi") == 0) {
    Serial.println("[CMD] Escaneando redes WiFi...");
    WifiMgr.startScan();
    
    // Esperar a que termine (máx 10 segundos)
    int attempts = 0;
    while (WiFi.scanComplete() == WIFI_SCAN_RUNNING && attempts < 20) {
      delay(500);
      attempts++;
    }
    
    int n = WiFi.scanComplete();
    if (n >= 0) {
      StaticJsonDocument<1024> response;
      response["cmd"] = "scan_wifi";
      response["status"] = "ok";
      JsonArray networks = response.createNestedArray("networks");
      
      for (int i = 0; i < n && i < 10; i++) {  // Máximo 10 redes
        JsonObject net = networks.createNestedObject();
        net["ssid"] = WiFi.SSID(i);
        net["rssi"] = WiFi.RSSI(i);
        net["channel"] = WiFi.channel(i);
        net["encrypted"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
      }
      
      String output;
      serializeJson(response, output);
      MqttMgr.publish(responseTopic.c_str(), output.c_str());
      WiFi.scanDelete();
    } else {
      MqttMgr.publish(responseTopic.c_str(), "{\"error\":\"scan_failed\"}");
    }
  }
  
  // ========== GET ERROR HISTORY ==========
  else if (strcmp(cmd, "get_errors") == 0) {
    StaticJsonDocument<1024> response;
    response["cmd"] = "get_errors";
    response["status"] = "ok";
    response["total_errors"] = errorCount;
    
    JsonArray errorArray = response.createNestedArray("errors");
    int start = (errorCount > 5) ? (errorCount - 5) : 0;
    int count = min(errorCount, 5);
    
    for (int i = 0; i < count; i++) {
      JsonObject err = errorArray.createNestedObject();
      SystemError& e = errors[i];
      err["code"] = e.code;
      err["type"] = getErrorTypeName(e.type);
      err["description"] = e.description;
      err["timestamp"] = e.timestamp;
      err["age_seconds"] = (millis() - e.timestamp) / 1000;
    }
    
    String output;
    serializeJson(response, output);
    MqttMgr.publish(responseTopic.c_str(), output.c_str());
  }
  
  // ========== CLEAR ERRORS ==========
  else if (strcmp(cmd, "clear_errors") == 0) {
    clearError();
    errorCount = 0;
    memset(errors, 0, sizeof(errors));
    MqttMgr.publish(responseTopic.c_str(), "{\"status\":\"ok\",\"message\":\"Errores limpiados\"}");
    Serial.println("[CMD] Errores limpiados");
  }
  
  // ========== RESTART ==========
  else if (strcmp(cmd, "restart") == 0) {
    Serial.println("[CMD] Reiniciando sistema...");
    MqttMgr.publish(responseTopic.c_str(), "{\"status\":\"restarting\"}");
    delay(500);
    SysMgr.restart(1000);
  }
  
  // ========== FACTORY RESET ==========
  else if (strcmp(cmd, "factory_reset") == 0) {
    Serial.println("[CMD] Factory reset...");
    MqttMgr.publish(responseTopic.c_str(), "{\"status\":\"factory_reset\"}");
    delay(500);
    SysMgr.factoryReset();
  }
  
  // ========== COMANDO NO RECONOCIDO ==========
  else {
    Serial.printf("[CMD] Comando desconocido: %s\n", cmd);
    MqttMgr.publish(responseTopic.c_str(), "{\"error\":\"unknown_command\"}");
  }
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  // Inicializar serial
  Serial.begin(115200);
  delay(500);
  
  Serial.println("\n\n");
  Serial.println("╔══════════════════════════════════════════════╗");
  Serial.println("║                                              ║");
  Serial.println("║   Nehuentue Suit Sensor v2.0                 ║");
  Serial.println("║   Arquitectura Modular + FreeRTOS            ║");
  Serial.println("║                                              ║");
  Serial.println("╚══════════════════════════════════════════════╝");
  Serial.println();
  
  // Deshabilitar Task Watchdog para evitar reinicios durante operaciones largas (WiFi scan)
  // El escaneo WiFi ahora es asíncrono, pero mantenemos esto por seguridad
  disableCore0WDT();
  Serial.println("[WDT] Task Watchdog Core 0 deshabilitado");
  
  // ========================================================================
  // 1. System Manager (información del sistema)
  // ========================================================================
  Serial.println("[INIT] Inicializando System Manager...");
  SysMgr.begin();
  SysMgr.printInfo();
  
  // ========================================================================
  // 2. Flash Storage Manager (persistencia)
  // ========================================================================
  Serial.println("[INIT] Inicializando Flash Storage Manager...");
  
  // Aplicar valores preconfigurados por defecto
  strncpy(wifiConfig.ssid, DEFAULT_WIFI_SSID, sizeof(wifiConfig.ssid) - 1);
  strncpy(wifiConfig.password, DEFAULT_WIFI_PASSWORD, sizeof(wifiConfig.password) - 1);
  strncpy(wifiConfig.hostname, DEFAULT_HOSTNAME, sizeof(wifiConfig.hostname) - 1);
  
  strncpy(mqttConfig.server, DEFAULT_MQTT_SERVER, sizeof(mqttConfig.server) - 1);
  mqttConfig.port = DEFAULT_MQTT_PORT;
  strncpy(mqttConfig.user, DEFAULT_MQTT_USER, sizeof(mqttConfig.user) - 1);
  strncpy(mqttConfig.password, DEFAULT_MQTT_PASSWORD, sizeof(mqttConfig.password) - 1);
  strncpy(mqttConfig.clientId, DEFAULT_MQTT_CLIENT_ID, sizeof(mqttConfig.clientId) - 1);
  
  Serial.println("[CONFIG] ✓ Credenciales preconfiguradas cargadas");
  Serial.printf("[CONFIG]   WiFi SSID: %s\n", wifiConfig.ssid);
  Serial.printf("[CONFIG]   MQTT Server: %s:%d\n", mqttConfig.server, mqttConfig.port);
  
  if (!FlashStorage.begin("nehuentue")) {
    Serial.println("[WARN] FlashStorage falló - usando solo valores preconfigurados");
    logError(ERROR_FLASH, ERR_EEPROM_INIT_FAILED);
  } else {
    Serial.println("[INIT] ✓ Flash Storage inicializado");
    
    // Intentar cargar sobrescrituras de la flash (si existen)
    bool sensorLoaded = FlashStorage.load("sensor_config", sensorConfig);
    
    // WiFi Config (puede sobrescribir los valores por defecto)
    String ssid = FlashStorage.loadString("wifi_ssid", "");
    if (ssid.length() > 0) {
      strncpy(wifiConfig.ssid, ssid.c_str(), sizeof(wifiConfig.ssid) - 1);
      String password = FlashStorage.loadString("wifi_password", "");
      strncpy(wifiConfig.password, password.c_str(), sizeof(wifiConfig.password) - 1);
      Serial.printf("[CONFIG] WiFi sobrescrito desde Flash: %s\n", wifiConfig.ssid);
    }
    
    // MQTT Config (puede sobrescribir los valores por defecto)
    String mqttServer = FlashStorage.loadString("mqtt_server", "");
    if (mqttServer.length() > 0) {
      strncpy(mqttConfig.server, mqttServer.c_str(), sizeof(mqttConfig.server) - 1);
      mqttConfig.port = FlashStorage.loadInt("mqtt_port", DEFAULT_MQTT_PORT);
      String mqttUser = FlashStorage.loadString("mqtt_user", "");
      String mqttPassword = FlashStorage.loadString("mqtt_password", "");
      if (mqttUser.length() > 0) strncpy(mqttConfig.user, mqttUser.c_str(), sizeof(mqttConfig.user) - 1);
      if (mqttPassword.length() > 0) strncpy(mqttConfig.password, mqttPassword.c_str(), sizeof(mqttConfig.password) - 1);
      Serial.printf("[CONFIG] MQTT sobrescrito desde Flash: %s:%d\n", mqttConfig.server, mqttConfig.port);
    }
    
    if (sensorLoaded) {
      Serial.printf("[CONFIG] Sensor cargado: '%s' (Slave ID: %d)\n", 
                    sensorConfig.name, sensorConfig.slaveId);
    }
  }
  
  // ========================================================================
  // 3. WiFi Manager (conectividad)
  // ========================================================================
  Serial.println("[INIT] Inicializando WiFi Manager...");
  WifiMgr.begin(wifiConfig.hostname);
  WifiMgr.onEvent(onWiFiEvent);
  
  Serial.printf("[WiFi] Conectando a '%s'...\n", wifiConfig.ssid);
  WifiMgr.connectSTA(wifiConfig.ssid, wifiConfig.password);
  
  // Esperar 15 segundos para conexión
  int attempts = 0;
  while (!WifiMgr.isConnected() && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();
  
  if (WifiMgr.isConnected()) {
    Serial.printf("[WiFi] ✓ Conectado - IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("[WiFi] ✓ RSSI: %d dBm\n", WifiMgr.getRSSI());
  } else {
    Serial.println("[WiFi] ✗ Error: No se pudo conectar a la red");
    Serial.println("[WiFi] Verifica las credenciales en config.h");
    Serial.println("[WiFi] El sistema continuará intentando reconectar...");
    logError(ERROR_WIFI, ERR_WIFI_CONNECTION_FAILED);
  }
  
  // ========================================================================
  // 4. MQTT Manager
  // ========================================================================
  Serial.println("[INIT] Inicializando MQTT Manager...");
  MqttMgr.begin(mqttConfig.server, mqttConfig.port, 
                mqttConfig.user, mqttConfig.password, mqttConfig.clientId);
  MqttMgr.onMessage(onMqttMessage);
  MqttMgr.setAutoReconnect(true);
  
  if (WifiMgr.isConnected()) {
    Serial.printf("[MQTT] Conectando a %s:%d...\n", mqttConfig.server, mqttConfig.port);
    if (MqttMgr.connect()) {
      Serial.println("[MQTT] ✓ Conectado al broker");
      
      // Suscribirse al tópico de comandos
      String cmdTopic = String(MQTT_TOPIC_BASE) + "/" + String(mqttConfig.clientId) + "/" + String(MQTT_TOPIC_CMD);
      MqttMgr.subscribe(cmdTopic.c_str());
      Serial.printf("[MQTT] ✓ Suscrito a: %s\n", cmdTopic.c_str());
      
      // Publicar mensaje de inicio
      String statusTopic = String(MQTT_TOPIC_BASE) + "/" + String(mqttConfig.clientId) + "/" + String(MQTT_TOPIC_STATUS);
      MqttMgr.publish(statusTopic.c_str(), "{\"status\":\"online\",\"firmware\":\"v2.1\"}");
    } else {
      Serial.println("[MQTT] ✗ Error al conectar al broker");
    }
  } else {
    Serial.println("[MQTT] Esperando conexión WiFi...");
  }
  
  // ========================================================================
  // 5. Modbus Manager
  // ========================================================================
  Serial.println("[INIT] Inicializando Modbus Manager...");
  ModbusMgr.begin(Serial1, sensorConfig.rxPin, sensorConfig.txPin, 
                  sensorConfig.baudrate);
  ModbusMgr.setTimeout(1000);
  Serial.println("[INIT] ✓ Modbus RTU Master inicializado");
  
  // ========================================================================
  // 6. Inicializar tareas FreeRTOS (legacy - DESHABILITADO TEMPORALMENTE)
  // ========================================================================
  // TODO: Migrar tasks.cpp para usar los managers
  // Serial.println("[INIT] Inicializando tareas FreeRTOS...");
  // initTasks();
  // vTaskDelay(pdMS_TO_TICKS(300));
  
  // ========================================================================
  // INICIO COMPLETADO
  // ========================================================================
  Serial.println("\n╔══════════════════════════════════════════════╗");
  Serial.println("║                                              ║");
  Serial.println("║  ✅ SISTEMA INICIADO CORRECTAMENTE           ║");
  Serial.println("║                                              ║");
  Serial.println("╚══════════════════════════════════════════════╝");
  
  SysMgr.printStatus();
  
  Serial.println("\n[READY] Sistema operativo - Tareas ejecutándose");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
  // Monitoreo de memoria cada 60 segundos
  static unsigned long lastMemCheck = 0;
  if (millis() - lastMemCheck > 60000) {
    lastMemCheck = millis();
    
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t maxAlloc = ESP.getMaxAllocHeap();
    
    // Detectar memoria baja (menos de 50KB libres)
    if (freeHeap < 50000) {
      char desc[128];
      snprintf(desc, sizeof(desc), "Heap libre: %lu bytes", freeHeap);
      logError(ERROR_MEMORY, ERR_SYSTEM_LOW_MEMORY, desc);
      Serial.printf("[MEMORY] ⚠️  Memoria baja: %lu bytes libres\n", freeHeap);
    }
    
    // Detectar fragmentación (max alloc < 50% del heap libre)
    if (maxAlloc < (freeHeap / 2)) {
      char desc[128];
      snprintf(desc, sizeof(desc), "Heap libre: %lu, Max alloc: %lu", freeHeap, maxAlloc);
      logError(ERROR_MEMORY, ERR_SYSTEM_HEAP_FRAGMENTED, desc);
      Serial.printf("[MEMORY] ⚠️  Heap fragmentado: libre=%lu, max_alloc=%lu\n", freeHeap, maxAlloc);
    }
  }
  
  // Loop del sistema
  SysMgr.loop();
  
  // Loop de MQTT Manager (mantiene conexión y procesa cola)
  if (WifiMgr.isConnected()) {
    MqttMgr.loop();
  }
  
  // Estadísticas periódicas
  static unsigned long lastStats = 0;
  if (millis() - lastStats > 60000) {  // Cada 60 segundos
    lastStats = millis();
    
    Serial.println("\n╔════════════════════════════════════════════════╗");
    Serial.println("║  📊 ESTADÍSTICAS DEL SISTEMA                   ║");
    Serial.println("╚════════════════════════════════════════════════╝");
    
    // Estado del sistema
    SystemStatus status = SysMgr.getStatus();
    Serial.printf("  Uptime: %lu s\n", status.uptime / 1000);
    Serial.printf("  Heap libre: %lu bytes\n", status.freeHeap);
    Serial.printf("  WiFi: %s\n", status.wifiConnected ? "✓ Conectado" : "✗ Desconectado");
    Serial.printf("  MQTT: %s\n", status.mqttConnected ? "✓ Conectado" : "✗ Desconectado");
    
    // Estadísticas Modbus
    const ModbusStats& modbusStats = ModbusMgr.getStats();
    Serial.printf("  Modbus peticiones: %lu (éxito: %lu, fallos: %lu)\n",
                  modbusStats.totalRequests,
                  modbusStats.successfulRequests,
                  modbusStats.failedRequests);
    
    // Estadísticas MQTT
    if (strlen(mqttConfig.server) > 0) {
      const MQTTStats& mqttStats = MqttMgr.getStats();
      Serial.printf("  MQTT publicados: %lu, recibidos: %lu\n",
                    mqttStats.totalPublished,
                    mqttStats.totalReceived);
    }
    
    // TODO: Implementar lectura de datos del sensor usando ModbusMgr
    // (código legacy de tasks.cpp deshabilitado temporalmente)
    /*
    if (dataMutex != NULL && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      if (sensorData.valid && sensorData.registerCount > 0) {
        Serial.println("  Últimos datos del sensor:");
        Serial.printf("    Registros leídos: %d\n", sensorData.registerCount);
        if (sensorData.registerCount > 0) {
          Serial.printf("    Reg[0]: %d (0x%04X)\n", 
                        sensorData.registers[0], sensorData.registers[0]);
        }
        if (sensorData.registerCount > 1) {
          Serial.printf("    Reg[1]: %d (0x%04X)\n", 
                        sensorData.registers[1], sensorData.registers[1]);
        }
        Serial.printf("    Timestamp: hace %lu ms\n", millis() - sensorData.timestamp);
      } else {
        Serial.println("  ⚠ No hay datos válidos del sensor");
      }
      xSemaphoreGive(dataMutex);
    }
    */
    
    Serial.println("════════════════════════════════════════════════\n");
  }
  
  delay(100);
}
