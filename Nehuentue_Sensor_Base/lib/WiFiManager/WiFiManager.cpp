/**
 * @file WiFiManager.cpp
 * @brief Implementación del WiFiManager
 * @version 1.0.0
 * @date 2025-10-19
 */

#include "WiFiManager.h"

// Instancia global
WiFiManager WifiMgr;
WiFiManager* WiFiManager::instance = nullptr;

// ============================================================================
// CONSTRUCTOR Y DESTRUCTOR
// ============================================================================

WiFiManager::WiFiManager() {
    initialized = false;
    wifiTaskHandle = NULL;
    mutex = NULL;
    eventGroup = NULL;
    currentMode = WIFI_MODE_NULL;  // Modo ESP-IDF para WiFi apagado
    currentStatus = WIFI_STATUS_IDLE;
    lastReconnectAttempt = 0;
    connectionStartTime = 0;
    autoReconnectEnabled = true;
    eventCallback = nullptr;
    
    memset(&config, 0, sizeof(WiFiConfig));
    memset(&stats, 0, sizeof(WiFiManagerStats));
    
    instance = this;  // Para callbacks estáticos
}

WiFiManager::~WiFiManager() {
    end();
}

// ============================================================================
// INICIALIZACIÓN
// ============================================================================

bool WiFiManager::begin(const char* hostname) {
    if (initialized) {
        Serial.println("[WIFI MGR] Ya inicializado");
        return true;
    }
    
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║   WiFi Manager v1.0                    ║");
    Serial.println("╚════════════════════════════════════════╝");
    
    // Crear mutex
    mutex = xSemaphoreCreateMutex();
    if (mutex == NULL) {
        Serial.println("[WIFI MGR] ERROR: No se pudo crear mutex");
        return false;
    }
    
    // Crear event group
    eventGroup = xEventGroupCreate();
    if (eventGroup == NULL) {
        Serial.println("[WIFI MGR] ERROR: No se pudo crear event group");
        vSemaphoreDelete(mutex);
        return false;
    }
    
    // Configurar hostname
    if (hostname != nullptr) {
        strncpy(config.hostname, hostname, sizeof(config.hostname) - 1);
    } else {
        // Generar hostname basado en MAC
        uint8_t mac[6];
        WiFi.macAddress(mac);
        snprintf(config.hostname, sizeof(config.hostname), "ESP32-%02X%02X%02X", mac[3], mac[4], mac[5]);
    }
    
    config.autoReconnect = true;
    config.reconnectInterval = WIFI_MANAGER_RECONNECT_INTERVAL;
    
    // Configurar WiFi mode inicial
    WiFi.mode(WIFI_MODE_NULL);
    
    // Registrar eventos WiFi
    WiFi.onEvent(onWiFiEvent);
    
    initialized = true;
    
    Serial.printf("  Hostname: %s\n", config.hostname);
    Serial.printf("  MAC: %s\n", WiFi.macAddress().c_str());
    Serial.printf("  Auto-reconnect: %s\n", config.autoReconnect ? "Enabled" : "Disabled");
    Serial.println("════════════════════════════════════════\n");
    
    return true;
}

void WiFiManager::end() {
    if (initialized) {
        // Detener todo
        disconnect();
        stopAP();
        
        WiFi.mode(WIFI_MODE_NULL);
        
        initialized = false;
        Serial.println("[WIFI MGR] Finalizado");
    }
    
    if (mutex != NULL) {
        vSemaphoreDelete(mutex);
        mutex = NULL;
    }
    
    if (eventGroup != NULL) {
        vEventGroupDelete(eventGroup);
        eventGroup = NULL;
    }
}

// ============================================================================
// MODO ACCESS POINT (AP)
// ============================================================================

bool WiFiManager::startAP(const char* ssid, const char* password, int channel, bool hidden, int maxConnections) {
    if (!initialized) {
        Serial.println("[WIFI MGR] ERROR: No inicializado");
        return false;
    }
    
    lock();
    
    Serial.println("[WIFI MGR] Iniciando Access Point...");
    Serial.printf("  SSID: %s\n", ssid);
    Serial.printf("  Password: %s\n", strlen(password) > 0 ? "********" : "(abierto)");
    Serial.printf("  Channel: %d\n", channel);
    
    // Configurar modo
    WiFi.mode(WIFI_AP);
    
    // Configurar AP
    bool success;
    if (strlen(password) >= 8) {
        success = WiFi.softAP(ssid, password, channel, hidden, maxConnections);
    } else {
        success = WiFi.softAP(ssid, nullptr, channel, hidden, maxConnections);
    }
    
    if (success) {
        currentMode = WIFI_MODE_AP;
        currentStatus = WIFI_STATUS_AP_RUNNING;
        xEventGroupSetBits(eventGroup, WIFI_AP_STARTED_BIT);
        
        IPAddress ip = WiFi.softAPIP();
        Serial.printf("  ✓ AP iniciado\n");
        Serial.printf("  IP: %s\n", ip.toString().c_str());
        
        // Notificar evento de AP iniciado
        if (eventCallback != nullptr) {
            arduino_event_info_t info;
            eventCallback(ARDUINO_EVENT_WIFI_AP_START, info);
        }
    } else {
        Serial.println("  ✗ Error al iniciar AP");
    }
    
    unlock();
    return success;
}

void WiFiManager::stopAP() {
    if (currentMode == WIFI_MODE_AP || currentMode == WIFI_MODE_APSTA) {
        lock();
        
        Serial.println("[WIFI MGR] Deteniendo Access Point...");
        WiFi.softAPdisconnect(true);
        xEventGroupClearBits(eventGroup, WIFI_AP_STARTED_BIT);
        
        if (currentMode == WIFI_MODE_AP) {
            currentMode = WIFI_MODE_NULL;  // Modo ESP-IDF para WiFi apagado
            currentStatus = WIFI_STATUS_IDLE;
        }
        
        // Notificar evento de AP detenido
        if (eventCallback != nullptr) {
            arduino_event_info_t info;
            eventCallback(ARDUINO_EVENT_WIFI_AP_STOP, info);
        }
        
        unlock();
    }
}

IPAddress WiFiManager::getAPIP() {
    return WiFi.softAPIP();
}

int WiFiManager::getAPClientCount() {
    return WiFi.softAPgetStationNum();
}

// ============================================================================
// MODO STATION (STA)
// ============================================================================

bool WiFiManager::connectSTA(const char* ssid, const char* password, bool waitForConnection) {
    if (!initialized) {
        Serial.println("[WIFI MGR] ERROR: No inicializado");
        return false;
    }
    
    lock();
    
    // Guardar configuración
    strncpy(config.ssid, ssid, sizeof(config.ssid) - 1);
    strncpy(config.password, password, sizeof(config.password) - 1);
    
    Serial.println("[WIFI MGR] Conectando a WiFi...");
    Serial.printf("  SSID: %s\n", ssid);
    
    // Configurar modo
    if (currentMode == WIFI_MODE_AP) {
        WiFi.mode(WIFI_AP_STA);
        currentMode = WIFI_MODE_APSTA;
    } else {
        WiFi.mode(WIFI_STA);
        currentMode = WIFI_MODE_STA;
    }
    
    // Configurar hostname
    WiFi.setHostname(config.hostname);
    
    // Iniciar conexión
    currentStatus = WIFI_STATUS_CONNECTING;
    connectionStartTime = millis();
    stats.connectionAttempts++;
    
    WiFi.begin(ssid, password);
    
    unlock();
    
    if (waitForConnection) {
        return this->waitForConnection(WIFI_MANAGER_CONNECTION_TIMEOUT);
    }
    
    return true;
}

void WiFiManager::disconnect() {
    if (currentMode == WIFI_MODE_STA || currentMode == WIFI_MODE_APSTA) {
        lock();
        
        Serial.println("[WIFI MGR] Desconectando WiFi...");
        WiFi.disconnect(true);
        xEventGroupClearBits(eventGroup, WIFI_CONNECTED_BIT);
        
        if (currentMode == WIFI_MODE_STA) {
            currentMode = WIFI_MODE_NULL;  // Modo ESP-IDF para WiFi apagado
        }
        currentStatus = WIFI_STATUS_DISCONNECTED;
        
        unlock();
    }
}

bool WiFiManager::isConnected() {
    return (xEventGroupGetBits(eventGroup) & WIFI_CONNECTED_BIT) != 0;
}

IPAddress WiFiManager::getIP() {
    return WiFi.localIP();
}

IPAddress WiFiManager::getGateway() {
    return WiFi.gatewayIP();
}

IPAddress WiFiManager::getSubnet() {
    return WiFi.subnetMask();
}

IPAddress WiFiManager::getDNS() {
    return WiFi.dnsIP();
}

int8_t WiFiManager::getRSSI() {
    if (isConnected()) {
        return WiFi.RSSI();
    }
    return 0;
}

// ============================================================================
// SCAN DE REDES
// ============================================================================

bool WiFiManager::startScan() {
    if (!initialized) return false;
    
    lock();
    xEventGroupSetBits(eventGroup, WIFI_SCANNING_BIT);
    unlock();
    
    Serial.println("[WIFI MGR] Escaneando redes WiFi...");
    WiFi.scanNetworks(true);  // Async
    
    return true;
}

int WiFiManager::getScanResults(WiFiNetworkInfo* networks, int maxNetworks) {
    int n = WiFi.scanComplete();
    
    if (n == WIFI_SCAN_RUNNING) {
        return 0;
    }
    
    if (n == WIFI_SCAN_FAILED) {
        Serial.println("[WIFI MGR] Scan failed");
        return 0;
    }
    
    int count = min(n, maxNetworks);
    
    for (int i = 0; i < count; i++) {
        strncpy(networks[i].ssid, WiFi.SSID(i).c_str(), 32);
        networks[i].ssid[32] = '\0';
        networks[i].rssi = WiFi.RSSI(i);
        networks[i].encryption = WiFi.encryptionType(i);
        networks[i].channel = WiFi.channel(i);
    }
    
    xEventGroupClearBits(eventGroup, WIFI_SCANNING_BIT);
    
    // Notificar evento de escaneo completado
    if (eventCallback != nullptr) {
        arduino_event_info_t info;
        eventCallback(ARDUINO_EVENT_WIFI_SCAN_DONE, info);
    }
    
    return count;
}

int WiFiManager::getScanCount() {
    int n = WiFi.scanComplete();
    return (n >= 0) ? n : 0;
}

// ============================================================================
// CONFIGURACIÓN
// ============================================================================

void WiFiManager::setHostname(const char* hostname) {
    lock();
    strncpy(config.hostname, hostname, sizeof(config.hostname) - 1);
    WiFi.setHostname(config.hostname);
    unlock();
}

String WiFiManager::getHostname() {
    return String(config.hostname);
}

void WiFiManager::setAutoReconnect(bool enable) {
    lock();
    config.autoReconnect = enable;
    autoReconnectEnabled = enable;
    unlock();
}

void WiFiManager::setReconnectInterval(uint32_t intervalMs) {
    lock();
    config.reconnectInterval = intervalMs;
    unlock();
}

bool WiFiManager::setStaticIP(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns1) {
    if (!WiFi.config(ip, gateway, subnet, dns1)) {
        Serial.println("[WIFI MGR] ERROR: Fallo al configurar IP estática");
        return false;
    }
    Serial.printf("[WIFI MGR] IP estática configurada: %s\n", ip.toString().c_str());
    return true;
}

void WiFiManager::enableDHCP() {
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    Serial.println("[WIFI MGR] DHCP habilitado");
}

// ============================================================================
// INFORMACIÓN
// ============================================================================

String WiFiManager::getMacAddress() {
    return WiFi.macAddress();
}

String WiFiManager::getSSID() {
    return WiFi.SSID();
}

wifi_mode_t WiFiManager::getMode() {
    return currentMode;
}

WiFiManagerStatus WiFiManager::getStatus() {
    return currentStatus;
}

void WiFiManager::resetStats() {
    lock();
    memset(&stats, 0, sizeof(WiFiManagerStats));
    unlock();
}

void WiFiManager::printStats() {
    lock();
    
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║   WiFi Manager - Estadísticas          ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.printf("  Conexiones exitosas: %lu\n", stats.totalConnections);
    Serial.printf("  Desconexiones: %lu\n", stats.totalDisconnections);
    Serial.printf("  Intentos de conexión: %lu\n", stats.connectionAttempts);
    Serial.printf("  Intentos fallidos: %lu\n", stats.failedAttempts);
    Serial.printf("  Última conexión: %lu ms\n", stats.lastConnectTime);
    Serial.printf("  Última desconexión: %lu ms\n", stats.lastDisconnectTime);
    Serial.printf("  Uptime total: %lu ms\n", stats.totalUptime);
    
    if (isConnected()) {
        Serial.printf("  RSSI actual: %d dBm\n", getRSSI());
    }
    
    Serial.println("════════════════════════════════════════\n");
    
    unlock();
}

void WiFiManager::printInfo() {
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║   WiFi Manager - Información           ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.printf("  Hostname: %s\n", config.hostname);
    Serial.printf("  MAC: %s\n", getMacAddress().c_str());
    Serial.printf("  Modo: %d\n", currentMode);
    Serial.printf("  Estado: %d\n", currentStatus);
    
    if (currentMode == WIFI_MODE_AP || currentMode == WIFI_MODE_APSTA) {
        Serial.printf("  AP IP: %s\n", getAPIP().toString().c_str());
        Serial.printf("  AP Clients: %d\n", getAPClientCount());
    }
    
    if (isConnected()) {
        Serial.printf("  SSID: %s\n", getSSID().c_str());
        Serial.printf("  IP: %s\n", getIP().toString().c_str());
        Serial.printf("  Gateway: %s\n", getGateway().toString().c_str());
        Serial.printf("  Subnet: %s\n", getSubnet().toString().c_str());
        Serial.printf("  DNS: %s\n", getDNS().toString().c_str());
        Serial.printf("  RSSI: %d dBm\n", getRSSI());
    }
    
    Serial.println("════════════════════════════════════════\n");
}

// ============================================================================
// CALLBACKS
// ============================================================================

void WiFiManager::onEvent(WiFiEventCallback callback) {
    lock();
    eventCallback = callback;
    unlock();
}

// ============================================================================
// UTILIDADES
// ============================================================================

bool WiFiManager::waitForConnection(uint32_t timeoutMs) {
    unsigned long start = millis();
    
    Serial.println("[WIFI MGR] Esperando conexión...");
    
    while (!isConnected() && (millis() - start < timeoutMs)) {
        delay(100);
    }
    
    if (isConnected()) {
        Serial.println("[WIFI MGR] ✓ Conectado");
        return true;
    } else {
        Serial.println("[WIFI MGR] ✗ Timeout");
        return false;
    }
}

const char* WiFiManager::encryptionTypeToString(uint8_t encType) {
    switch (encType) {
        case WIFI_AUTH_OPEN: return "OPEN";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-E";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
        default: return "UNKNOWN";
    }
}

// ============================================================================
// MÉTODOS PRIVADOS
// ============================================================================

void WiFiManager::lock() {
    if (mutex != NULL) {
        xSemaphoreTake(mutex, portMAX_DELAY);
    }
}

void WiFiManager::unlock() {
    if (mutex != NULL) {
        xSemaphoreGive(mutex);
    }
}

void WiFiManager::onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    if (instance == nullptr) return;
    
    instance->lock();
    
    // Notificar al usuario con el evento nativo de ESP-IDF
    if (instance->eventCallback != nullptr) {
        instance->eventCallback(event, info);
    }
    
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_START:
            Serial.println("[WIFI MGR] STA Started");
            break;
            
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("[WIFI MGR] STA Connected");
            instance->stats.totalConnections++;
            instance->stats.lastConnectTime = millis();
            break;
            
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.printf("[WIFI MGR] ✓ WiFi conectado\n");
            Serial.printf("  IP: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("  RSSI: %d dBm\n", WiFi.RSSI());
            
            xEventGroupSetBits(instance->eventGroup, WIFI_CONNECTED_BIT);
            instance->currentStatus = WIFI_STATUS_CONNECTED;
            instance->stats.currentRSSI = WiFi.RSSI();
            break;
            
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("[WIFI MGR] ✗ WiFi desconectado");
            
            xEventGroupClearBits(instance->eventGroup, WIFI_CONNECTED_BIT);
            instance->currentStatus = WIFI_STATUS_DISCONNECTED;
            instance->stats.totalDisconnections++;
            instance->stats.lastDisconnectTime = millis();
            
            // Auto-reconnect
            if (instance->autoReconnectEnabled) {
                instance->lastReconnectAttempt = millis();
            }
            break;
            
        case ARDUINO_EVENT_WIFI_AP_START:
            Serial.println("[WIFI MGR] AP Started");
            break;
            
        case ARDUINO_EVENT_WIFI_AP_STOP:
            Serial.println("[WIFI MGR] AP Stopped");
            break;
            
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            Serial.println("[WIFI MGR] Cliente conectado al AP");
            break;
            
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            Serial.println("[WIFI MGR] Cliente desconectado del AP");
            break;
            
        default:
            break;
    }
    
    instance->unlock();
}
