# 📡 WiFiManager

**Librería thread-safe para gestión WiFi en ESP32 con FreeRTOS**

Versión: 1.0.0  
Fecha: 19 de octubre de 2025

---

## 📋 Características

- ✅ **Thread-safe** con mutexes FreeRTOS
- ✅ **Modo AP** (Access Point) para configuración
- ✅ **Modo STA** (Station) con auto-reconnect
- ✅ **Modo AP+STA** (ambos simultáneamente)
- ✅ **Callbacks** para eventos de conexión
- ✅ **Scan** de redes WiFi disponibles
- ✅ **IP estática** o DHCP
- ✅ **Estadísticas** de conexión
- ✅ **Hostname** configurable

---

## 🚀 Uso Básico

### 1. Inicialización

```cpp
#include <WiFiManager.h>

void setup() {
    WifiMgr.begin("mi-esp32");  // Hostname opcional
}
```

### 2. Modo Access Point (Configuración)

```cpp
// AP abierto
WifiMgr.startAP("ESP32-Config");

// AP con contraseña
WifiMgr.startAP("ESP32-Config", "password123");

// AP personalizado
WifiMgr.startAP("MiDispositivo", "12345678", 6, false, 4);
//                 ssid         password    ch hidden maxClients

// Obtener IP del AP
IPAddress ip = WifiMgr.getAPIP();
Serial.printf("AP IP: %s\n", ip.toString().c_str());
```

### 3. Modo Station (Conectar a red)

```cpp
// Conectar (async)
WifiMgr.connectSTA("Mi_Red_WiFi", "mi_password");

// Conectar y esperar
WifiMgr.connectSTA("Mi_Red_WiFi", "mi_password", true);

// Verificar conexión
if (WifiMgr.isConnected()) {
    IPAddress ip = WifiMgr.getIP();
    Serial.printf("Conectado! IP: %s\n", ip.toString().c_str());
}
```

### 4. Callbacks de Eventos

```cpp
WifiMgr.onEvent([](WiFiManagerEvent event) {
    switch (event) {
        case WIFI_EVENT_STA_CONNECTED:
            Serial.println("WiFi conectado!");
            break;
            
        case WIFI_EVENT_STA_GOT_IP:
            Serial.printf("IP: %s\n", WifiMgr.getIP().toString().c_str());
            break;
            
        case WIFI_EVENT_STA_DISCONNECTED:
            Serial.println("WiFi desconectado");
            break;
            
        case WIFI_EVENT_AP_START:
            Serial.println("AP iniciado");
            break;
    }
});
```

---

## 🎯 Ejemplos Completos

### Ejemplo 1: Configuración vía AP → STA

```cpp
#include <WiFiManager.h>

void setup() {
    Serial.begin(115200);
    
    // Inicializar
    WifiMgr.begin("sensor-01");
    
    // Callback de eventos
    WifiMgr.onEvent([](WiFiManagerEvent event) {
        if (event == WIFI_EVENT_STA_GOT_IP) {
            Serial.println("✓ Conectado a WiFi!");
            WifiMgr.printInfo();
        }
    });
    
    // Intentar conectar a red guardada
    if (!WifiMgr.connectSTA("Mi_Red", "password", true)) {
        Serial.println("No se pudo conectar, iniciando modo AP");
        
        // Iniciar AP para configuración
        WifiMgr.startAP("Sensor-Config", "12345678");
        Serial.println("Conéctate a 'Sensor-Config' para configurar");
    }
}

void loop() {
    delay(1000);
}
```

### Ejemplo 2: Scan de Redes

```cpp
void scanWiFiNetworks() {
    WifiMgr.startScan();
    
    // Esperar a que termine el scan
    delay(3000);
    
    // Obtener resultados
    WiFiNetworkInfo networks[20];
    int count = WifiMgr.getScanResults(networks, 20);
    
    Serial.printf("Redes encontradas: %d\n", count);
    for (int i = 0; i < count; i++) {
        Serial.printf("%d. %s (%d dBm) [%s]\n", 
                     i + 1,
                     networks[i].ssid,
                     networks[i].rssi,
                     WiFiManager::encryptionTypeToString(networks[i].encryption));
    }
}
```

### Ejemplo 3: Auto-reconnect

```cpp
void setup() {
    WifiMgr.begin();
    
    // Configurar auto-reconnect
    WifiMgr.setAutoReconnect(true);
    WifiMgr.setReconnectInterval(5000);  // 5 segundos
    
    // Conectar
    WifiMgr.connectSTA("Mi_Red", "password");
    
    // Los eventos de reconexión se manejan automáticamente
}
```

### Ejemplo 4: IP Estática

```cpp
void setup() {
    WifiMgr.begin();
    
    // Configurar IP estática ANTES de conectar
    IPAddress ip(192, 168, 1, 100);
    IPAddress gateway(192, 168, 1, 1);
    IPAddress subnet(255, 255, 255, 0);
    IPAddress dns(8, 8, 8, 8);
    
    WifiMgr.setStaticIP(ip, gateway, subnet, dns);
    
    // Conectar
    WifiMgr.connectSTA("Mi_Red", "password");
}
```

---

## 📊 API Completa

### Inicialización
```cpp
bool begin(const char* hostname = nullptr);
void end();
bool isReady() const;
```

### Access Point
```cpp
bool startAP(const char* ssid, const char* password = "", int channel = 1, bool hidden = false, int maxConnections = 4);
void stopAP();
IPAddress getAPIP();
int getAPClientCount();
```

### Station
```cpp
bool connectSTA(const char* ssid, const char* password, bool waitForConnection = false);
void disconnect();
bool isConnected();
IPAddress getIP();
IPAddress getGateway();
IPAddress getSubnet();
IPAddress getDNS();
int8_t getRSSI();
```

### Scan
```cpp
bool startScan();
int getScanResults(WiFiNetworkInfo* networks, int maxNetworks);
int getScanCount();
```

### Configuración
```cpp
void setHostname(const char* hostname);
String getHostname();
void setAutoReconnect(bool enable);
void setReconnectInterval(uint32_t intervalMs);
bool setStaticIP(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns1 = IPAddress(0,0,0,0));
void enableDHCP();
```

### Información
```cpp
String getMacAddress();
String getSSID();
WiFiManagerMode getMode();
WiFiManagerStatus getStatus();
WiFiManagerStats getStats() const;
void resetStats();
void printStats();
void printInfo();
```

### Callbacks
```cpp
void onEvent(WiFiEventCallback callback);
```

### Utilidades
```cpp
bool waitForConnection(uint32_t timeoutMs = 30000);
static const char* encryptionTypeToString(uint8_t encType);
```

---

## 🔧 Estructuras

### WiFiNetworkInfo
```cpp
struct WiFiNetworkInfo {
    char ssid[33];
    int8_t rssi;
    uint8_t encryption;
    uint8_t channel;
};
```

### WiFiManagerStats
```cpp
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
```

---

## 📈 Eventos

```cpp
enum WiFiManagerEvent {
    WIFI_EVENT_STA_START,
    WIFI_EVENT_STA_CONNECTED,
    WIFI_EVENT_STA_DISCONNECTED,
    WIFI_EVENT_STA_GOT_IP,
    WIFI_EVENT_STA_LOST_IP,
    WIFI_EVENT_AP_START,
    WIFI_EVENT_AP_STOP,
    WIFI_EVENT_AP_STACONNECTED,
    WIFI_EVENT_AP_STADISCONNECTED,
    WIFI_EVENT_SCAN_DONE
};
```

---

## ⚠️ Notas Importantes

### Auto-Reconnect
- Habilitado por defecto
- Intervalo configurable (default: 5 segundos)
- Se activa automáticamente al desconectar

### Thread-Safety
- Todos los métodos son thread-safe
- Usa mutexes FreeRTOS internamente
- Seguro llamar desde múltiples tareas

### Modos Simultáneos
- Puedes tener AP y STA activos al mismo tiempo
- Útil para configuración sin perder conectividad

---

## 🐛 Troubleshooting

### "STA enable failed"
- Problema: Error al iniciar modo Station
- Solución: Verificar que no haya otro manager WiFi activo

### Auto-reconnect no funciona
- Verificar: `WifiMgr.setAutoReconnect(true)`
- Verificar: Intervalo de reconnect adecuado

### Scan no devuelve resultados
- Esperar al menos 2-3 segundos después de `startScan()`
- Verificar con `getScanCount()` si terminó

---

## 📚 Integración con Proyecto

Ver `MODULAR_ARCHITECTURE_PROGRESS.md` para integración completa.

---

## 🔗 Referencias

- [ESP32 WiFi API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html)
- [Arduino WiFi Library](https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi)
- [FreeRTOS Event Groups](https://www.freertos.org/event-groups-API.html)
