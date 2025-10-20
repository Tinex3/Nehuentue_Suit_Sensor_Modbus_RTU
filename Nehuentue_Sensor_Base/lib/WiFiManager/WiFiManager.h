/**
 * @file WiFiManager.h
 * @brief Manager para conexión WiFi (AP/STA) con FreeRTOS
 * @version 1.0.0
 * @date 2025-10-19
 * 
 * @details
 * Librería thread-safe para gestionar conexión WiFi en ESP32.
 * Soporta modo Access Point (AP) y Station (STA) con auto-reconnect.
 * 
 * Características:
 * - Thread-safe con mutexes FreeRTOS
 * - Modo AP para configuración inicial
 * - Modo STA con auto-reconnect
 * - Callbacks para eventos de conexión
 * - Scan de redes WiFi disponibles
 * - Gestión de hostname y MAC address
 * - Estadísticas de conexión
 * 
 * Uso:
 * @code
 * WiFiManager wifi;
 * wifi.begin();
 * 
 * // Modo AP para configuración
 * wifi.startAP("MyDevice-XXXX", "password123");
 * 
 * // Modo STA para conectar
 * wifi.connectSTA("Mi_Red", "mi_password");
 * 
 * // Callback de eventos
 * wifi.onEvent([](WiFiManagerEvent event) {
 *     if (event == WIFI_EVENT_STA_CONNECTED) {
 *         Serial.println("WiFi conectado!");
 *     }
 * });
 * @endcode
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

// ============================================================================
// CONSTANTES Y CONFIGURACIÓN
// ============================================================================

#define WIFI_MANAGER_VERSION "1.0.0"
#define WIFI_MANAGER_TASK_STACK_SIZE 4096
#define WIFI_MANAGER_TASK_PRIORITY 2
#define WIFI_MANAGER_RECONNECT_INTERVAL 5000    // 5 segundos
#define WIFI_MANAGER_SCAN_INTERVAL 60000        // 60 segundos
#define WIFI_MANAGER_CONNECTION_TIMEOUT 30000   // 30 segundos
#define WIFI_MANAGER_MAX_SSID_LENGTH 32
#define WIFI_MANAGER_MAX_PASSWORD_LENGTH 64
#define WIFI_MANAGER_MAX_HOSTNAME_LENGTH 32

// Event Group bits
#define WIFI_CONNECTED_BIT    BIT0
#define WIFI_AP_STARTED_BIT   BIT1
#define WIFI_SCANNING_BIT     BIT2

// ============================================================================
// NOTA: Usando tipos nativos de ESP-IDF
// ============================================================================
// Se usan directamente:
// - arduino_event_id_t (WiFiEvent_t) para eventos WiFi
// - wifi_mode_t para modos de operación
// Esto evita conflictos con las definiciones del ESP-IDF

/**
 * @brief Estados de conexión
 */
enum WiFiManagerStatus {
    WIFI_STATUS_IDLE,
    WIFI_STATUS_CONNECTING,
    WIFI_STATUS_CONNECTED,
    WIFI_STATUS_DISCONNECTED,
    WIFI_STATUS_CONNECTION_FAILED,
    WIFI_STATUS_AP_RUNNING
};

// ============================================================================
// ESTRUCTURAS
// ============================================================================

/**
 * @brief Información de red WiFi escaneada
 */
struct WiFiNetworkInfo {
    char ssid[33];
    int8_t rssi;
    uint8_t encryption;
    uint8_t channel;
};

/**
 * @brief Estadísticas de conexión WiFi
 */
struct WiFiManagerStats {
    uint32_t totalConnections;
    uint32_t totalDisconnections;
    uint32_t connectionAttempts;
    uint32_t failedAttempts;
    unsigned long lastConnectTime;
    unsigned long lastDisconnectTime;
    unsigned long totalUptime;
    int8_t currentRSSI;
};

/**
 * @brief Configuración WiFi
 */
struct WiFiConfig {
    char ssid[WIFI_MANAGER_MAX_SSID_LENGTH];
    char password[WIFI_MANAGER_MAX_PASSWORD_LENGTH];
    char hostname[WIFI_MANAGER_MAX_HOSTNAME_LENGTH];
    bool autoReconnect;
    uint32_t reconnectInterval;
};

// ============================================================================
// CALLBACK TYPES
// ============================================================================

// Usa directamente el tipo de evento de Arduino/ESP32-IDF
typedef std::function<void(arduino_event_id_t event, arduino_event_info_t info)> WiFiEventCallback;

// ============================================================================
// CLASE PRINCIPAL
// ============================================================================

/**
 * @brief Manager para conexión WiFi con FreeRTOS
 */
class WiFiManager {
public:
    // ========================================================================
    // CONSTRUCTOR Y DESTRUCTOR
    // ========================================================================
    
    WiFiManager();
    ~WiFiManager();
    
    // ========================================================================
    // INICIALIZACIÓN
    // ========================================================================
    
    /**
     * @brief Inicializa el WiFi manager
     * @param hostname Nombre del host (opcional)
     * @return true si se inicializó correctamente
     */
    bool begin(const char* hostname = nullptr);
    
    /**
     * @brief Finaliza y libera recursos
     */
    void end();
    
    /**
     * @brief Verifica si está inicializado
     */
    bool isReady() const { return initialized; }
    
    // ========================================================================
    // MODO ACCESS POINT (AP)
    // ========================================================================
    
    /**
     * @brief Inicia modo Access Point
     * @param ssid SSID del AP
     * @param password Contraseña (min 8 chars, vacío para abierto)
     * @param channel Canal WiFi (1-13)
     * @param hidden Ocultar SSID
     * @param maxConnections Máximo de clientes (1-4)
     * @return true si se inició correctamente
     */
    bool startAP(const char* ssid, 
                 const char* password = "", 
                 int channel = 1,
                 bool hidden = false,
                 int maxConnections = 4);
    
    /**
     * @brief Detiene el modo Access Point
     */
    void stopAP();
    
    /**
     * @brief Obtiene IP del AP
     */
    IPAddress getAPIP();
    
    /**
     * @brief Obtiene número de clientes conectados al AP
     */
    int getAPClientCount();
    
    // ========================================================================
    // MODO STATION (STA)
    // ========================================================================
    
    /**
     * @brief Conecta a red WiFi en modo Station
     * @param ssid SSID de la red
     * @param password Contraseña
     * @param waitForConnection Esperar hasta conectar
     * @return true si se inició la conexión (o conectó si wait=true)
     */
    bool connectSTA(const char* ssid, 
                    const char* password,
                    bool waitForConnection = false);
    
    /**
     * @brief Desconecta de red WiFi
     */
    void disconnect();
    
    /**
     * @brief Verifica si está conectado en modo STA
     */
    bool isConnected();
    
    /**
     * @brief Obtiene IP asignada en modo STA
     */
    IPAddress getIP();
    
    /**
     * @brief Obtiene gateway
     */
    IPAddress getGateway();
    
    /**
     * @brief Obtiene subnet mask
     */
    IPAddress getSubnet();
    
    /**
     * @brief Obtiene DNS
     */
    IPAddress getDNS();
    
    /**
     * @brief Obtiene RSSI (señal) actual
     */
    int8_t getRSSI();
    
    // ========================================================================
    // SCAN DE REDES
    // ========================================================================
    
    /**
     * @brief Escanea redes WiFi disponibles (async)
     * @return true si se inició el scan
     */
    bool startScan();
    
    /**
     * @brief Obtiene resultado del último scan
     * @param networks Array donde guardar resultados
     * @param maxNetworks Tamaño máximo del array
     * @return Número de redes encontradas
     */
    int getScanResults(WiFiNetworkInfo* networks, int maxNetworks);
    
    /**
     * @brief Obtiene número de redes del último scan
     */
    int getScanCount();
    
    // ========================================================================
    // CONFIGURACIÓN
    // ========================================================================
    
    /**
     * @brief Establece hostname
     */
    void setHostname(const char* hostname);
    
    /**
     * @brief Obtiene hostname actual
     */
    String getHostname();
    
    /**
     * @brief Habilita/deshabilita auto-reconnect
     */
    void setAutoReconnect(bool enable);
    
    /**
     * @brief Establece intervalo de reconnect (ms)
     */
    void setReconnectInterval(uint32_t intervalMs);
    
    /**
     * @brief Configura IP estática (STA mode)
     */
    bool setStaticIP(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns1 = IPAddress(0,0,0,0));
    
    /**
     * @brief Vuelve a DHCP
     */
    void enableDHCP();
    
    // ========================================================================
    // INFORMACIÓN
    // ========================================================================
    
    /**
     * @brief Obtiene MAC address como string
     */
    String getMacAddress();
    
    /**
     * @brief Obtiene SSID actual (STA mode)
     */
    String getSSID();
    
    /**
     * @brief Obtiene modo actual
     * @return wifi_mode_t Modo WiFi actual (WIFI_MODE_STA, WIFI_MODE_AP, etc.)
     */
    wifi_mode_t getMode();
    
    /**
     * @brief Obtiene estado actual
     */
    WiFiManagerStatus getStatus();
    
    /**
     * @brief Obtiene estadísticas
     */
    WiFiManagerStats getStats() const { return stats; }
    
    /**
     * @brief Resetea estadísticas
     */
    void resetStats();
    
    /**
     * @brief Imprime estadísticas
     */
    void printStats();
    
    /**
     * @brief Imprime información de conexión
     */
    void printInfo();
    
    // ========================================================================
    // CALLBACKS
    // ========================================================================
    
    /**
     * @brief Registra callback para eventos WiFi
     */
    void onEvent(WiFiEventCallback callback);
    
    // ========================================================================
    // UTILIDADES
    // ========================================================================
    
    /**
     * @brief Espera hasta que se conecte (con timeout)
     * @param timeoutMs Timeout en milisegundos
     * @return true si se conectó
     */
    bool waitForConnection(uint32_t timeoutMs = WIFI_MANAGER_CONNECTION_TIMEOUT);
    
    /**
     * @brief Convierte tipo de encriptación a string
     */
    static const char* encryptionTypeToString(uint8_t encType);

private:
    // ========================================================================
    // VARIABLES PRIVADAS
    // ========================================================================
    
    bool initialized;
    TaskHandle_t wifiTaskHandle;
    SemaphoreHandle_t mutex;
    EventGroupHandle_t eventGroup;
    
    wifi_mode_t currentMode;
    WiFiManagerStatus currentStatus;
    WiFiConfig config;
    WiFiManagerStats stats;
    WiFiEventCallback eventCallback;
    
    unsigned long lastReconnectAttempt;
    unsigned long connectionStartTime;
    bool autoReconnectEnabled;
    
    // ========================================================================
    // MÉTODOS PRIVADOS
    // ========================================================================
    
    static void wifiTask(void* parameter);
    void handleWiFiEvents();
    void handleReconnect();
    
    static void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info);
    static WiFiManager* instance;  // Para callbacks estáticos
    
    void lock();
    void unlock();
};

// ============================================================================
// INSTANCIA GLOBAL (opcional)
// ============================================================================
extern WiFiManager WifiMgr;

#endif // WIFI_MANAGER_H
