#include "tasks.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

// Variables compartidas
SensorData sensorData;
SemaphoreHandle_t dataMutex = NULL;
WiFiConfig wifiConfig;
MQTTTopics mqttTopics;

// Colas para comunicación entre tareas
QueueHandle_t modbusQueue = NULL;
// QueueHandle_t eepromQueue = NULL;  // EEPROM deshabilitada temporalmente

// Estructura para datos crudos en la cola
struct RawModbusData {
    bool valid;
    uint8_t data[256];
    size_t length;
    unsigned long timestamp;
};

// =============================================================================
// TAREA 1: MODBUS - Lee datos del sensor via Modbus RTU
// =============================================================================
void modbusTask(void *pvParameters) {
    Serial.println("[MODBUS TASK] Iniciada");
    
    const TickType_t pollingInterval = pdMS_TO_TICKS(5000); // 5 segundos
    TickType_t lastWakeTime = xTaskGetTickCount();
    
    for (;;) {
        Serial.println("\n[MODBUS TASK] Leyendo sensor...");
        
        // Lee 2 registros holding desde dirección 0 del esclavo ID 1
        ModbusResponse response = modbusReadHoldingRegisters(1, 0, 2);
        
        // Prepara datos para enviar a decoder
        RawModbusData rawData;
        rawData.valid = response.success;
        rawData.length = response.length;
        rawData.timestamp = millis();
        memcpy(rawData.data, response.data, response.length);
        
        // Envía datos a la cola del decoder
        if (modbusQueue != NULL) {
            if (xQueueSend(modbusQueue, &rawData, pdMS_TO_TICKS(100)) == pdTRUE) {
                Serial.println("[MODBUS TASK] Datos enviados a decoder");
            } else {
                Serial.println("[MODBUS TASK] ERROR: Cola llena");
            }
        }
        
        // Espera hasta el próximo ciclo
        vTaskDelayUntil(&lastWakeTime, pollingInterval);
    }
}

// =============================================================================
// TAREA 2: DECODER - Decodifica datos del sensor
// =============================================================================
void decoderTask(void *pvParameters) {
    Serial.println("[DECODER TASK] Iniciada");
    
    RawModbusData rawData;
    
    for (;;) {
        // Espera datos de la cola de Modbus
        if (xQueueReceive(modbusQueue, &rawData, portMAX_DELAY) == pdTRUE) {
            Serial.println("\n[DECODER TASK] Procesando datos...");
            
            if (rawData.valid && rawData.length >= 9) {
                // Decodifica la respuesta Modbus
                // Formato: [SlaveID][Function][ByteCount][Data...][CRC]
                uint8_t byteCount = rawData.data[2];
                
                if (byteCount >= 4) {
                    // Extrae registros (16 bits cada uno, Big Endian)
                    uint16_t reg0 = (rawData.data[3] << 8) | rawData.data[4];
                    uint16_t reg1 = (rawData.data[5] << 8) | rawData.data[6];
                    
                    // Decodifica según tu sensor específico
                    // Ejemplo: temperatura y humedad
                    float temperature = reg0 / 10.0;  // Ajusta según tu sensor
                    float humidity = reg1 / 10.0;     // Ajusta según tu sensor
                    
                    Serial.printf("[DECODER TASK] Registro 0: %d (0x%04X)\n", reg0, reg0);
                    Serial.printf("[DECODER TASK] Registro 1: %d (0x%04X)\n", reg1, reg1);
                    Serial.printf("[DECODER TASK] Temperatura: %.1f °C\n", temperature);
                    Serial.printf("[DECODER TASK] Humedad: %.1f %%\n", humidity);
                    
                    // Guarda datos procesados (protegido por mutex)
                    if (dataMutex != NULL && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                        sensorData.valid = true;
                        sensorData.register0 = reg0;
                        sensorData.register1 = reg1;
                        sensorData.temperature = temperature;
                        sensorData.humidity = humidity;
                        sensorData.timestamp = rawData.timestamp;
                        xSemaphoreGive(dataMutex);
                        
                        Serial.println("[DECODER TASK] Datos actualizados");
                    }
                } else {
                    Serial.println("[DECODER TASK] ERROR: Byte count inválido");
                }
            } else {
                Serial.println("[DECODER TASK] ERROR: Datos inválidos o incompletos");
                
                // Marca datos como inválidos
                if (dataMutex != NULL && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    sensorData.valid = false;
                    xSemaphoreGive(dataMutex);
                }
            }
        }
    }
}

// =============================================================================
// FUNCIÓN AUXILIAR: Construye los tópicos MQTT dinámicamente
// =============================================================================
void buildMQTTTopics(const char* deviceId) {
    snprintf(mqttTopics.telemetryTemp, sizeof(mqttTopics.telemetryTemp), 
             "devices/%s/telemetry/temperature", deviceId);
    
    snprintf(mqttTopics.telemetryCurrent, sizeof(mqttTopics.telemetryCurrent), 
             "devices/%s/telemetry/current", deviceId);
    
    snprintf(mqttTopics.status, sizeof(mqttTopics.status), 
             "devices/%s/status", deviceId);
    
    snprintf(mqttTopics.eventError, sizeof(mqttTopics.eventError), 
             "devices/%s/event/error", deviceId);
    
    snprintf(mqttTopics.cmdBase, sizeof(mqttTopics.cmdBase), 
             "devices/%s/cmd/#", deviceId);
    
    Serial.println("\n[MQTT] Tópicos construidos:");
    Serial.printf("  Telemetría Temp: %s\n", mqttTopics.telemetryTemp);
    Serial.printf("  Telemetría Current: %s\n", mqttTopics.telemetryCurrent);
    Serial.printf("  Estado: %s\n", mqttTopics.status);
    Serial.printf("  Eventos: %s\n", mqttTopics.eventError);
    Serial.printf("  Comandos: %s\n\n", mqttTopics.cmdBase);
}

// =============================================================================
// TAREA 3: MQTT - Gestiona conexión WiFi y publicación MQTT
// =============================================================================
void mqttTask(void *pvParameters) {
    Serial.println("[MQTT TASK] Iniciada");
    
    // TODO: Incluir librerías WiFi y PubSubClient
    // #include <WiFi.h>
    // #include <PubSubClient.h>
    // WiFiClient espClient;
    // PubSubClient mqttClient(espClient);
    
    bool wifiConnected = false;
    bool mqttConnected = false;
    
    const TickType_t checkInterval = pdMS_TO_TICKS(5000);   // Verifica conexión cada 5s
    const TickType_t publishInterval = pdMS_TO_TICKS(10000); // Publica cada 10s
    unsigned long lastPublish = 0;
    unsigned long lastStatus = 0;
    
    // Construye los tópicos MQTT
    buildMQTTTopics(wifiConfig.deviceId);
    
    // Intenta conectar WiFi
    Serial.println("[MQTT TASK] Conectando a WiFi...");
    Serial.printf("  SSID: %s\n", wifiConfig.ssid);
    // WiFi.begin(wifiConfig.ssid, wifiConfig.password);
    // WiFi.setHostname(wifiConfig.deviceId);
    
    for (;;) {
        // ============== GESTIÓN WiFi ==============
        // if (WiFi.status() != WL_CONNECTED) {
        //     if (wifiConnected) {
        //         Serial.println("[MQTT TASK] WiFi desconectado, reconectando...");
        //         mqttConnected = false;
        //     }
        //     wifiConnected = false;
        //     WiFi.reconnect();
        //     vTaskDelay(pdMS_TO_TICKS(1000));
        //     continue;
        // }
        
        // if (!wifiConnected) {
        //     wifiConnected = true;
        //     Serial.println("[MQTT TASK] ✓ WiFi conectado");
        //     Serial.printf("  IP: %s\n", WiFi.localIP().toString().c_str());
        //     Serial.printf("  RSSI: %d dBm\n", WiFi.RSSI());
        // }
        
        // ============== GESTIÓN MQTT ==============
        // if (!mqttClient.connected()) {
        //     if (mqttConnected) {
        //         Serial.println("[MQTT TASK] MQTT desconectado, reconectando...");
        //     }
        //     mqttConnected = false;
        //     
        //     mqttClient.setServer(wifiConfig.mqttServer, wifiConfig.mqttPort);
        //     
        //     // Conecta con usuario y contraseña
        //     String clientId = String(wifiConfig.deviceId) + "-" + String(ESP.getEfuseMac(), HEX);
        //     
        //     if (mqttClient.connect(clientId.c_str(), 
        //                           wifiConfig.mqttUser, 
        //                           wifiConfig.mqttPassword,
        //                           mqttTopics.status,  // Last Will Topic
        //                           1,                   // Last Will QoS
        //                           true,                // Last Will Retain
        //                           "{\"status\":\"offline\"}")) {
        //         
        //         mqttConnected = true;
        //         Serial.println("[MQTT TASK] ✓ MQTT conectado");
        //         Serial.printf("  Broker: %s:%d\n", wifiConfig.mqttServer, wifiConfig.mqttPort);
        //         Serial.printf("  Usuario: %s\n", wifiConfig.mqttUser);
        //         Serial.printf("  Client ID: %s\n", clientId.c_str());
        //         
        //         // Suscribirse a comandos
        //         mqttClient.subscribe(mqttTopics.cmdBase);
        //         Serial.printf("  Suscrito a: %s\n", mqttTopics.cmdBase);
        //         
        //         // Publicar estado online
        //         char statusPayload[128];
        //         snprintf(statusPayload, sizeof(statusPayload),
        //                 "{\"status\":\"online\",\"ip\":\"%s\",\"version\":\"1.0.0\"}",
        //                 WiFi.localIP().toString().c_str());
        //         mqttClient.publish(mqttTopics.status, statusPayload, true);
        //         
        //     } else {
        //         Serial.printf("[MQTT TASK] Error MQTT: %d\n", mqttClient.state());
        //         vTaskDelay(pdMS_TO_TICKS(5000));
        //         continue;
        //     }
        // }
        
        // mqttClient.loop();
        
        // ============== PRUEBA: Publicar "Hello World" ==============
        if (millis() - lastPublish >= 10000) {
            lastPublish = millis();
            
            // MODO DE PRUEBA: Publica "Hello World" simple
            char payload[128];
            snprintf(payload, sizeof(payload), 
                     "{\"message\":\"Hello World\",\"uptime\":%lu,\"timestamp\":%lu}",
                     millis() / 1000, millis());
            
            Serial.println("[MQTT TASK] Publicando Hello World...");
            Serial.printf("  Tópico: %s\n", mqttTopics.telemetryTemp);
            Serial.printf("  Payload: %s\n", payload);
            
            // if (mqttClient.publish(mqttTopics.telemetryTemp, payload)) {
            //     Serial.println("[MQTT TASK] ✓ Publicado correctamente");
            // } else {
            //     Serial.println("[MQTT TASK] ✗ Error al publicar");
            // }
            
            // BLOQUEO: Ciclo while infinito que escribe en serial
            Serial.println("[MQTT TASK] ENTRANDO EN CICLO BLOQUEANTE...");
            while (true) {
                Serial.println("Estoy aqui");
                delay(500);  // Medio segundo entre mensajes
            }
        }
        
        // ============== Publicar estado cada 60s ==============
        if (millis() - lastStatus >= 60000) {
            lastStatus = millis();
            
            char statusPayload[256];
            snprintf(statusPayload, sizeof(statusPayload),
                     "{\"status\":\"online\",\"uptime\":%lu,\"heap\":%u,\"rssi\":%d}",
                     millis() / 1000, ESP.getFreeHeap(), -65);  // WiFi.RSSI()
            
            Serial.println("[MQTT TASK] Publicando estado...");
            Serial.printf("  Payload: %s\n", statusPayload);
            
            // if (mqttClient.publish(mqttTopics.status, statusPayload, true)) {
            //     Serial.println("[MQTT TASK] ✓ Estado publicado");
            // }
        }
        
        vTaskDelay(checkInterval);
    }
}

// =============================================================================
// TAREA 4: EEPROM - Gestiona lectura/escritura de EEPROM con API genérica
// =============================================================================
void eepromTask(void *pvParameters) {
    Serial.println("[EEPROM TASK] Iniciada");
    
    EEPROMRequest request;
    
    // Intenta cargar configuración WiFi guardada
    Serial.println("[EEPROM TASK] Cargando configuración WiFi...");
    EEPROMStatus status = EEPROM24LC64.loadWithCRC<WiFiConfig>(EEPROM_ADDR_WIFI_CONFIG, wifiConfig);
    
    if (status == EEPROM_OK) {
        Serial.println("[EEPROM TASK] ✓ Configuración WiFi cargada con CRC válido");
        Serial.printf("  SSID: %s\n", wifiConfig.ssid);
        Serial.printf("  Device ID: %s\n", wifiConfig.deviceId);
        Serial.printf("  MQTT Server: %s:%d\n", wifiConfig.mqttServer, wifiConfig.mqttPort);
        Serial.printf("  MQTT User: %s\n", wifiConfig.mqttUser);
    } else {
        Serial.println("[EEPROM TASK] No hay configuración guardada o CRC inválido, usando por defecto");
        strcpy(wifiConfig.ssid, "MiWiFi");
        strcpy(wifiConfig.password, "password123");
        strcpy(wifiConfig.deviceId, "modbus-01");  // ID del dispositivo
        strcpy(wifiConfig.mqttServer, "192.168.1.25");  // IP de tu RPi
        wifiConfig.mqttPort = 1883;
        strcpy(wifiConfig.mqttUser, "mqttuser");
        strcpy(wifiConfig.mqttPassword, "1234");
        
        // Guarda configuración por defecto
        EEPROM24LC64.saveWithCRC<WiFiConfig>(EEPROM_ADDR_WIFI_CONFIG, wifiConfig);
        Serial.println("[EEPROM TASK] Configuración por defecto guardada");
    }
    
    // Intenta cargar últimos datos del sensor
    StoredSensorData lastData;
    status = EEPROM24LC64.loadWithCRC<StoredSensorData>(EEPROM_ADDR_SENSOR_DATA, lastData);
    
    if (status == EEPROM_OK) {
        Serial.println("[EEPROM TASK] Últimos datos del sensor guardados:");
        Serial.printf("  Temperatura: %.1f °C\n", lastData.temperature);
        Serial.printf("  Humedad: %.1f %%\n", lastData.humidity);
        Serial.printf("  Timestamp: %lu\n", lastData.timestamp);
    } else {
        Serial.println("[EEPROM TASK] No hay datos previos del sensor");
    }
    
    const TickType_t saveInterval = pdMS_TO_TICKS(60000); // Guarda cada 60 segundos
    TickType_t lastSave = xTaskGetTickCount();
    
    for (;;) {
        // Procesa comandos de la cola si hay
        if (xQueueReceive(eepromQueue, &request, pdMS_TO_TICKS(100)) == pdTRUE) {
            Serial.printf("[EEPROM TASK] Comando recibido: %d\n", request.command);
            
            switch (request.command) {
                case EEPROM_CMD_WRITE_SENSOR_DATA:
                    // Escribe datos del sensor con CRC
                    if (dataMutex != NULL && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                        if (sensorData.valid) {
                            StoredSensorData dataToSave;
                            dataToSave.temperature = sensorData.temperature;
                            dataToSave.humidity = sensorData.humidity;
                            dataToSave.timestamp = sensorData.timestamp;
                            
                            status = EEPROM24LC64.saveWithCRC<StoredSensorData>(EEPROM_ADDR_SENSOR_DATA, dataToSave);
                            if (status == EEPROM_OK) {
                                Serial.println("[EEPROM TASK] ✓ Datos del sensor guardados con CRC");
                            } else {
                                Serial.println("[EEPROM TASK] ✗ Error guardando datos del sensor");
                            }
                        }
                        xSemaphoreGive(dataMutex);
                    }
                    break;
                    
                case EEPROM_CMD_WRITE_CONFIG:
                    // Guarda configuración WiFi con CRC
                    status = EEPROM24LC64.saveWithCRC<WiFiConfig>(EEPROM_ADDR_WIFI_CONFIG, wifiConfig);
                    if (status == EEPROM_OK) {
                        Serial.println("[EEPROM TASK] ✓ Configuración WiFi guardada con CRC");
                    } else {
                        Serial.println("[EEPROM TASK] ✗ Error guardando configuración");
                    }
                    break;
                    
                default:
                    Serial.println("[EEPROM TASK] Comando desconocido");
                    break;
            }
        }
        
        // Guarda datos automáticamente cada 60 segundos
        if ((xTaskGetTickCount() - lastSave) >= saveInterval) {
            lastSave = xTaskGetTickCount();
            
            if (dataMutex != NULL && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                if (sensorData.valid) {
                    StoredSensorData dataToSave;
                    dataToSave.temperature = sensorData.temperature;
                    dataToSave.humidity = sensorData.humidity;
                    dataToSave.timestamp = sensorData.timestamp;
                    
                    status = EEPROM24LC64.saveWithCRC<StoredSensorData>(EEPROM_ADDR_SENSOR_DATA, dataToSave);
                    if (status == EEPROM_OK) {
                        Serial.println("[EEPROM TASK] ✓ Auto-guardado de datos del sensor con CRC");
                    }
                }
                xSemaphoreGive(dataMutex);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// =============================================================================
// FUNCIÓN: Inicializa configuración WiFi/MQTT por defecto (sin EEPROM)
// =============================================================================
void initDefaultConfig() {
    Serial.println("[CONFIG] Inicializando configuración por defecto (sin EEPROM)...");
    
    strcpy(wifiConfig.ssid, "Amanda 2.4G");
    strcpy(wifiConfig.password, "Gomezriquelmegomez12");
    strcpy(wifiConfig.deviceId, "modbus-01");  // ID del dispositivo
    strcpy(wifiConfig.mqttServer, "192.168.1.25");  // IP de tu RPi
    wifiConfig.mqttPort = 1883;
    strcpy(wifiConfig.mqttUser, "mqttuser");
    strcpy(wifiConfig.mqttPassword, "1234");
    
    Serial.println("[CONFIG] ✓ Configuración cargada:");
    Serial.printf("  SSID: %s\n", wifiConfig.ssid);
    Serial.printf("  Device ID: %s\n", wifiConfig.deviceId);
    Serial.printf("  MQTT Server: %s:%d\n", wifiConfig.mqttServer, wifiConfig.mqttPort);
    Serial.printf("  MQTT User: %s\n\n", wifiConfig.mqttUser);
}

// =============================================================================
// Inicialización de tareas
// =============================================================================
void initTasks() {
    Serial.println("Inicializando sistema de tareas FreeRTOS...");
    
    // Inicializa configuración por defecto (sin EEPROM)
    initDefaultConfig();
    
    // Crea mutex para proteger datos compartidos
    dataMutex = xSemaphoreCreateMutex();
    if (dataMutex == NULL) {
        Serial.println("ERROR: No se pudo crear dataMutex");
        return;
    }
    
    // Crea cola para pasar datos de Modbus a Decoder
    modbusQueue = xQueueCreate(5, sizeof(RawModbusData));
    if (modbusQueue == NULL) {
        Serial.println("ERROR: No se pudo crear modbusQueue");
        return;
    }
    
    // ========== EEPROM DESHABILITADA ==========
    // Crea cola para comandos de EEPROM
    // eepromQueue = xQueueCreate(10, sizeof(EEPROMRequest));
    // if (eepromQueue == NULL) {
    //     Serial.println("ERROR: No se pudo crear eepromQueue");
    //     return;
    // }
    // ==========================================
    
    // Inicializa datos compartidos
    sensorData.valid = false;
    sensorData.register0 = 0;
    sensorData.register1 = 0;
    sensorData.temperature = 0.0;
    sensorData.humidity = 0.0;
    sensorData.timestamp = 0;
    
    // Crea tarea 1: Modbus
    BaseType_t result1 = xTaskCreate(
        modbusTask,           // Función de la tarea
        "ModbusTask",         // Nombre
        4096,                 // Stack size
        NULL,                 // Parámetros
        2,                    // Prioridad (2 = alta)
        NULL                  // Handle
    );
    
    if (result1 != pdPASS) {
        Serial.println("ERROR: No se pudo crear ModbusTask");
    } else {
        Serial.println("✓ ModbusTask creada");
    }
    
    // Crea tarea 2: Decoder
    BaseType_t result2 = xTaskCreate(
        decoderTask,          // Función de la tarea
        "DecoderTask",        // Nombre
        4096,                 // Stack size
        NULL,                 // Parámetros
        2,                    // Prioridad (2 = alta)
        NULL                  // Handle
    );
    
    if (result2 != pdPASS) {
        Serial.println("ERROR: No se pudo crear DecoderTask");
    } else {
        Serial.println("✓ DecoderTask creada");
    }
    
    // Crea tarea 3: MQTT
    BaseType_t result3 = xTaskCreate(
        mqttTask,             // Función de la tarea
        "MQTT Task",          // Nombre
        8192,                 // Stack size (más grande para WiFi/MQTT)
        NULL,                 // Parámetros
        1,                    // Prioridad (1 = media)
        NULL                  // Handle
    );
    
    if (result3 != pdPASS) {
        Serial.println("ERROR: No se pudo crear MQTT Task");
    } else {
        Serial.println("✓ MQTT Task creada");
    }
    
    // ========== EEPROM TASK DESHABILITADA ==========
    // Crea tarea 4: EEPROM
    // BaseType_t result4 = xTaskCreate(
    //     eepromTask,           // Función de la tarea
    //     "EEPROM Task",        // Nombre
    //     4096,                 // Stack size
    //     NULL,                 // Parámetros
    //     1,                    // Prioridad (1 = media)
    //     NULL                  // Handle
    // );
    // 
    // if (result4 != pdPASS) {
    //     Serial.println("ERROR: No se pudo crear EEPROM Task");
    // } else {
    //     Serial.println("✓ EEPROM Task creada");
    // }
    Serial.println("(EEPROM Task deshabilitada - sin hardware)");
    // ===============================================
    
    Serial.println("Sistema de tareas inicializado correctamente");
}
