# Página Web de Configuración Interna - ESP32

## 📋 Descripción General

Interfaz web embebida en el ESP32 para configurar remotamente todos los parámetros del gateway Modbus-MQTT sin necesidad de recompilar firmware.

---

## 🎯 Objetivos

1. **Configuración WiFi**: SSID, password
2. **Configuración MQTT**: Server, port, usuario, contraseña, device ID
3. **Configuración Modbus**: Función, dirección esclavo, registro inicial, cantidad de registros
4. **Configuración de Sensores**: Tipo, unidad, multiplicador, offset, decimales
5. **Persistencia**: Guardar configuración en EEPROM
6. **Modo AP**: Access Point automático si no hay WiFi configurado

---

## 🏗️ Arquitectura

```
┌─────────────────────────────────────────────────────────────┐
│                    ESP32-C3 (QT Py)                         │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────┐         ┌──────────────────┐          │
│  │  WiFi Manager   │────────▶│  AsyncWebServer  │          │
│  │  - AP Mode      │         │  Port: 80        │          │
│  │  - STA Mode     │         └──────────────────┘          │
│  └─────────────────┘                 │                     │
│                                      │                     │
│                                      ▼                     │
│  ┌──────────────────────────────────────────────────────┐  │
│  │           Páginas Web (HTML/CSS/JS)                  │  │
│  │  - /                (Dashboard)                      │  │
│  │  - /wifi            (Configuración WiFi)             │  │
│  │  - /mqtt            (Configuración MQTT)             │  │
│  │  - /modbus          (Configuración Modbus)           │  │
│  │  - /sensors         (Configuración Sensores)         │  │
│  │  - /api/config      (GET/POST JSON config)           │  │
│  │  - /api/restart     (Reiniciar ESP32)                │  │
│  │  - /api/status      (Estado actual del sistema)      │  │
│  └──────────────────────────────────────────────────────┘  │
│                                      │                     │
│                                      ▼                     │
│  ┌──────────────────────────────────────────────────────┐  │
│  │              EEPROM Manager                          │  │
│  │  - Guardar configuración con CRC                     │  │
│  │  - Cargar configuración al inicio                    │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 📦 Dependencias Necesarias

### PlatformIO (platformio.ini)

```ini
[env:adafruit_qtpy_esp32c3]
platform = espressif32
board = adafruit_qtpy_esp32c3
framework = arduino

lib_deps = 
    knolleary/PubSubClient@^2.8
    me-no-dev/ESPAsyncWebServer@^1.2.3
    me-no-dev/AsyncTCP@^1.1.1

build_flags = 
    -D MQTT_MAX_PACKET_SIZE=1024
```

---

## 🌐 Modo Access Point (AP)

### Activación Automática

El ESP32 entrará en modo AP si:
- No hay configuración WiFi guardada en EEPROM
- Falla la conexión WiFi después de 30 segundos
- Se presiona botón BOOT durante el inicio (opcional)

### Configuración AP

```cpp
// Configuración por defecto
SSID:     "Modbus-Config-XXXX"  // XXXX = últimos 4 dígitos del MAC
Password: "12345678"             // Cambiar por defecto más seguro
IP:       192.168.4.1
Gateway:  192.168.4.1
Subnet:   255.255.255.0
```

### Acceso

1. Conectar a red WiFi: `Modbus-Config-XXXX`
2. Abrir navegador en: `http://192.168.4.1`
3. Configurar parámetros
4. Guardar → ESP32 reinicia y conecta a WiFi configurado

---

## 🖥️ Páginas Web

### 1. Dashboard Principal (`/`)

**Funciones:**
- Vista general del estado del sistema
- Última lectura de sensores
- Estado de conexión WiFi/MQTT
- Uptime del sistema
- Botón de reinicio
- Links a páginas de configuración

**Ejemplo de visualización:**
```
╔════════════════════════════════════════╗
║    Gateway Modbus-MQTT - ESP32-C3      ║
╠════════════════════════════════════════╣
║ Estado:     🟢 Online                  ║
║ WiFi:       Amanda 2.4G (-45 dBm)      ║
║ MQTT:       192.168.1.25:1883 ✓        ║
║ Uptime:     2h 34m 12s                 ║
║                                        ║
║ 📊 Sensores Activos:                   ║
║   • Temperature: 24.5 °C               ║
║   • Humidity:    65.2 %                ║
║                                        ║
║ [Configurar WiFi]  [Configurar MQTT]   ║
║ [Configurar Modbus] [Configurar Sens]  ║
║                                        ║
║             [🔄 Reiniciar]             ║
╚════════════════════════════════════════╝
```

---

### 2. Configuración WiFi (`/wifi`)

**Campos:**
```
┌─────────────────────────────────────┐
│ Configuración WiFi                  │
├─────────────────────────────────────┤
│ SSID:        [Amanda 2.4G________]  │
│ Password:    [••••••••••••••••••]   │
│ Device ID:   [modbus-01_________]   │
│                                     │
│ [Escanear Redes]                    │
│                                     │
│ Redes disponibles:                  │
│ ○ Amanda 2.4G         (-45 dBm) 🔒  │
│ ○ MiWiFi              (-67 dBm) 🔒  │
│ ○ Vecino_5G           (-82 dBm) 🔒  │
│                                     │
│        [Guardar]    [Cancelar]      │
└─────────────────────────────────────┘
```

**API Endpoint:** `POST /api/config/wifi`

```json
{
  "ssid": "Amanda 2.4G",
  "password": "Gomezriquelmegomez12",
  "device_id": "modbus-01"
}
```

---

### 3. Configuración MQTT (`/mqtt`)

**Campos:**
```
┌─────────────────────────────────────┐
│ Configuración MQTT                  │
├─────────────────────────────────────┤
│ Servidor:    [192.168.1.25______]   │
│ Puerto:      [1883______________]   │
│ Usuario:     [mqttuser__________]   │
│ Contraseña:  [••••________________] │
│                                     │
│ Tópicos base:                       │
│   devices/{device_id}/...           │
│                                     │
│ Intervalos (segundos):              │
│ Telemetría:  [60________________]   │
│ Estado:      [300_______________]   │
│                                     │
│ [Probar Conexión]                   │
│                                     │
│        [Guardar]    [Cancelar]      │
└─────────────────────────────────────┘
```

**API Endpoint:** `POST /api/config/mqtt`

```json
{
  "server": "192.168.1.25",
  "port": 1883,
  "user": "mqttuser",
  "password": "1234",
  "telemetry_interval": 60,
  "status_interval": 300
}
```

---

### 4. Configuración Modbus (`/modbus`)

**Campos:**
```
┌──────────────────────────────────────────────┐
│ Configuración Modbus                         │
├──────────────────────────────────────────────┤
│ Baudrate:          [9600▼]                   │
│ Data bits:         [8▼]                      │
│ Parity:            [None▼]                   │
│ Stop bits:         [1▼]                      │
│                                              │
│ GPIO RX (DE/RE):   [20________________]      │
│ GPIO TX (DI):      [21________________]      │
│                                              │
│ Intervalo polling: [5000__] ms              │
│                                              │
│ ℹ️ Configura sensores individuales en la    │
│    página "Configuración de Sensores"        │
│                                              │
│        [Guardar]    [Cancelar]               │
└──────────────────────────────────────────────┘
```

---

### 5. Configuración de Sensores (`/sensors`)

**Vista de lista + formulario por sensor:**

```
┌─────────────────────────────────────────────────────────┐
│ Configuración de Sensores (Máx: 4)                      │
├─────────────────────────────────────────────────────────┤
│                                                         │
│ [+] Agregar Nuevo Sensor                                │
│                                                         │
│ ┌───────────────────────────────────────────────────┐   │
│ │ Sensor #0: Temperature                       [✏️][🗑️]│
│ │ Tipo: temperature | Unidad: celsius               │   │
│ │ Modbus: Función 0x03, Addr 1, Reg 0, Count 1      │   │
│ │ Fórmula: (raw × 0.1) + 0                          │   │
│ │ Habilitado: ✅                                     │   │
│ └───────────────────────────────────────────────────┘   │
│                                                         │
│ ┌───────────────────────────────────────────────────┐   │
│ │ Sensor #1: Humidity                          [✏️][🗑️]│
│ │ Tipo: humidity | Unidad: percent                  │   │
│ │ Modbus: Función 0x03, Addr 1, Reg 1, Count 1      │   │
│ │ Fórmula: (raw × 0.1) + 0                          │   │
│ │ Habilitado: ✅                                     │   │
│ └───────────────────────────────────────────────────┘   │
│                                                         │
│                    [Guardar Todo]                       │
└─────────────────────────────────────────────────────────┘
```

**Formulario de edición (modal o página separada):**

```
┌─────────────────────────────────────────────────────┐
│ Editar Sensor #0                                    │
├─────────────────────────────────────────────────────┤
│                                                     │
│ Información General:                                │
│ Nombre:      [temperature_______________]           │
│ Tipo:        [temperature▼]                         │
│              - temperature, humidity, energy        │
│              - flow, pressure, voltage, current     │
│              - speed, level, custom                 │
│ Unidad:      [celsius___________________]           │
│                                                     │
│ Parámetros Modbus:                                  │
│ Función:     [0x03 - Read Holding▼]                │
│              - 0x03: Read Holding Registers         │
│              - 0x04: Read Input Registers           │
│ Dirección:   [1_____] (1-247)                       │
│ Reg Inicial: [0_____]                               │
│ Cant. Regs:  [1_____] (1-4)                         │
│                                                     │
│ Conversión de Datos:                                │
│ Multiplicador: [0.1_________________]               │
│ Offset:        [0___________________]               │
│ Decimales:     [2___] (0-6)                         │
│                                                     │
│ Fórmula resultante:                                 │
│   valor_final = (valor_raw × 0.1) + 0               │
│   Ejemplo: 245 → 24.5 celsius                       │
│                                                     │
│ Estado:                                             │
│ [x] Sensor habilitado                               │
│                                                     │
│        [Guardar]    [Cancelar]                      │
└─────────────────────────────────────────────────────┘
```

**API Endpoint:** `POST /api/config/sensors`

```json
{
  "sensors": [
    {
      "id": 0,
      "name": "temperature",
      "type": "temperature",
      "unit": "celsius",
      "modbus_address": 1,
      "modbus_function": 3,
      "register_start": 0,
      "register_count": 1,
      "multiplier": 0.1,
      "offset": 0.0,
      "decimals": 2,
      "enabled": true
    },
    {
      "id": 1,
      "name": "humidity",
      "type": "humidity",
      "unit": "percent",
      "modbus_address": 1,
      "modbus_function": 3,
      "register_start": 1,
      "register_count": 1,
      "multiplier": 0.1,
      "offset": 0.0,
      "decimals": 2,
      "enabled": true
    }
  ]
}
```

---

## 🔌 API REST Endpoints

### GET `/api/status`

Obtiene el estado actual completo del sistema.

**Response:**
```json
{
  "system": {
    "uptime": 9432,
    "free_heap": 145672,
    "firmware": "1.0.0",
    "chip_model": "ESP32-C3",
    "mac": "A8:42:E3:4D:2E:F0"
  },
  "wifi": {
    "connected": true,
    "ssid": "Amanda 2.4G",
    "ip": "192.168.1.142",
    "rssi": -45
  },
  "mqtt": {
    "connected": true,
    "server": "192.168.1.25",
    "port": 1883,
    "device_id": "modbus-01"
  },
  "modbus": {
    "baudrate": 9600,
    "gpio_rx": 20,
    "gpio_tx": 21
  },
  "sensors": [
    {
      "id": 0,
      "name": "temperature",
      "enabled": true,
      "last_value": 24.5,
      "unit": "celsius",
      "timestamp": 1729353245
    }
  ]
}
```

---

### GET `/api/config`

Obtiene la configuración actual (sin contraseñas).

**Response:**
```json
{
  "wifi": {
    "ssid": "Amanda 2.4G",
    "device_id": "modbus-01"
  },
  "mqtt": {
    "server": "192.168.1.25",
    "port": 1883,
    "user": "mqttuser",
    "telemetry_interval": 60,
    "status_interval": 300
  },
  "sensors": [...]
}
```

---

### POST `/api/config`

Actualiza toda la configuración de una vez.

**Request Body:**
```json
{
  "wifi": {...},
  "mqtt": {...},
  "sensors": [...]
}
```

**Response:**
```json
{
  "success": true,
  "message": "Configuración guardada. El dispositivo se reiniciará en 3 segundos.",
  "restart_in": 3000
}
```

---

### POST `/api/restart`

Reinicia el ESP32.

**Response:**
```json
{
  "success": true,
  "message": "Reiniciando en 2 segundos..."
}
```

---

### GET `/api/scan`

Escanea redes WiFi disponibles.

**Response:**
```json
{
  "networks": [
    {
      "ssid": "Amanda 2.4G",
      "rssi": -45,
      "encryption": "WPA2",
      "channel": 6
    },
    {
      "ssid": "MiWiFi",
      "rssi": -67,
      "encryption": "WPA2",
      "channel": 11
    }
  ]
}
```

---

## 🎨 Diseño UI/UX

### Paleta de Colores

```css
:root {
  --primary:    #2196F3;  /* Azul principal */
  --secondary:  #4CAF50;  /* Verde éxito */
  --danger:     #F44336;  /* Rojo error */
  --warning:    #FF9800;  /* Naranja advertencia */
  --dark:       #212121;  /* Fondo oscuro */
  --light:      #FAFAFA;  /* Fondo claro */
  --border:     #E0E0E0;  /* Bordes */
}
```

### Características

- **Responsive**: Funciona en móvil, tablet y desktop
- **Dark mode**: Opción de tema oscuro
- **Live updates**: WebSocket para actualización en tiempo real de valores
- **Validación**: Validación de campos antes de guardar
- **Feedback**: Notificaciones toast para éxito/error
- **Accesibilidad**: Labels, ARIA, contraste adecuado

---

## 🔐 Seguridad

### Consideraciones

1. **Contraseñas WiFi**: Nunca mostrar en GET, solo asteriscos
2. **HTTPS**: Opcional con certificado autofirmado
3. **Autenticación**: Usuario/contraseña para acceso web (opcional)
4. **CORS**: Restringir orígenes si es necesario
5. **Rate limiting**: Limitar requests por IP
6. **Timeout**: Sesión expira después de 15 minutos inactiva

### Autenticación Básica (Opcional)

```cpp
// Usuario y contraseña por defecto
const char* www_username = "admin";
const char* www_password = "modbus2024";

server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  if(!request->authenticate(www_username, www_password))
    return request->requestAuthentication();
  
  request->send(200, "text/html", index_html);
});
```

---

## 📁 Estructura de Archivos

```
src/
├── main.cpp                    # Punto de entrada
├── tasks.cpp                   # Tareas FreeRTOS (existente)
├── web_server.cpp              # Servidor web y API REST
├── web_server.h
└── html/
    ├── index.html              # Dashboard principal
    ├── wifi.html               # Configuración WiFi
    ├── mqtt.html               # Configuración MQTT
    ├── modbus.html             # Configuración Modbus
    ├── sensors.html            # Configuración Sensores
    ├── style.css               # Estilos globales
    └── app.js                  # JavaScript global

include/
├── tasks.h                     # Headers de tareas (existente)
├── web_server.h                # Headers del servidor web
└── html_pages.h                # Páginas HTML como strings

```

---

## 🚀 Flujo de Inicio

### Diagrama de Flujo

```
                    ┌──────────────┐
                    │  ESP32 Boot  │
                    └──────┬───────┘
                           │
                           ▼
                  ┌────────────────────┐
                  │ Inicializar EEPROM │
                  └────────┬───────────┘
                           │
                           ▼
                  ┌────────────────────┐
                  │ ¿Hay config WiFi?  │
                  └────────┬───────────┘
                           │
                 ┌─────────┴──────────┐
                 │                    │
                NO                   SÍ
                 │                    │
                 ▼                    ▼
        ┌────────────────┐   ┌──────────────────┐
        │  Modo AP       │   │ Conectar a WiFi  │
        │  192.168.4.1   │   └────────┬─────────┘
        └────────┬───────┘            │
                 │              ¿Conectó OK?
                 │                    │
                 │          ┌─────────┴──────────┐
                 │         SÍ                   NO
                 │          │                    │
                 │          ▼                    │
                 │  ┌───────────────┐            │
                 │  │ Modo STA      │            │
                 │  │ + Web Server  │            │
                 │  │ + MQTT Client │            │
                 │  └───────┬───────┘            │
                 │          │                    │
                 └──────────┴────────────────────┘
                            │
                            ▼
                   ┌────────────────┐
                   │  Web Server    │
                   │  activo en     │
                   │  puerto 80     │
                   └────────────────┘
```

---

## 🧪 Casos de Uso

### Caso 1: Primera Configuración

1. Usuario enciende ESP32 nuevo (sin config)
2. ESP32 entra en modo AP: `Modbus-Config-XXXX`
3. Usuario conecta su móvil a esa red WiFi
4. Abre navegador: `http://192.168.4.1`
5. Configura WiFi, MQTT y sensores
6. Presiona "Guardar"
7. ESP32 reinicia y conecta a WiFi configurado
8. Sistema operacional en modo normal

### Caso 2: Reconfiguración

1. Usuario conoce la IP del ESP32 en su red
2. Abre `http://192.168.1.142`
3. Modifica parámetros de sensores
4. Guarda cambios
5. ESP32 aplica cambios sin reiniciar (o reinicia si es WiFi/MQTT)

### Caso 3: WiFi Perdido

1. ESP32 pierde conexión WiFi
2. Intenta reconectar durante 30 segundos
3. Si falla, activa modo AP automáticamente
4. Usuario puede reconfigurar WiFi

---

## 🛠️ Implementación por Fases

### Fase 1: Servidor Web Básico ✅

- [ ] Agregar AsyncWebServer a platformio.ini
- [ ] Crear archivo `web_server.cpp/h`
- [ ] Implementar modo AP básico
- [ ] Página dashboard simple (HTML embebido)
- [ ] Endpoint `/api/status` con JSON

### Fase 2: Configuración WiFi/MQTT

- [ ] Página `/wifi` con formulario
- [ ] Endpoint `/api/config/wifi` (GET/POST)
- [ ] Página `/mqtt` con formulario
- [ ] Endpoint `/api/config/mqtt` (GET/POST)
- [ ] Persistir en EEPROM al guardar

### Fase 3: Configuración Modbus/Sensores

- [ ] Página `/sensors` con lista y formulario
- [ ] Endpoint `/api/config/sensors` (GET/POST)
- [ ] Validación de parámetros Modbus
- [ ] Preview de fórmula de conversión

### Fase 4: UI Avanzada

- [ ] Escaneo de redes WiFi (`/api/scan`)
- [ ] Gráficos en tiempo real (Chart.js)
- [ ] WebSocket para live updates
- [ ] Dark mode toggle
- [ ] Export/import de configuración JSON

### Fase 5: Seguridad y Optimización

- [ ] Autenticación HTTP Basic
- [ ] HTTPS con certificado autofirmado
- [ ] Compresión GZIP de assets
- [ ] Cache de páginas estáticas
- [ ] Rate limiting

---

## 📚 Recursos y Referencias

### Librerías ESP32

- **ESPAsyncWebServer**: https://github.com/me-no-dev/ESPAsyncWebServer
- **AsyncTCP**: https://github.com/me-no-dev/AsyncTCP
- **ArduinoJson**: https://arduinojson.org/
- **WiFiManager**: https://github.com/tzapu/WiFiManager (alternativa)

### Ejemplos

- ESP32 Web Server: https://randomnerdtutorials.com/esp32-web-server-arduino-ide/
- AsyncWebServer examples: https://github.com/me-no-dev/ESPAsyncWebServer/tree/master/examples

### UI Frameworks (Opcional)

- Bootstrap 5: https://getbootstrap.com/
- Tailwind CSS: https://tailwindcss.com/
- Material Design Lite: https://getmdl.io/

---

## 📝 Notas Adicionales

### Memoria Flash para HTML

Si el HTML/CSS/JS es muy grande:
- Usar SPIFFS o LittleFS para almacenar archivos
- Comprimir con GZIP antes de flashear
- Minificar HTML/CSS/JS

### Ejemplo de HTML minificado embebido:

```cpp
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Modbus Gateway</title><style>body{font-family:Arial;margin:0;padding:20px;background:#f5f5f5}.card{background:#fff;border-radius:8px;padding:20px;margin:10px 0;box-shadow:0 2px 4px rgba(0,0,0,.1)}</style></head><body><div class="card"><h1>Modbus Gateway</h1><p>Estado: <span id="status">Cargando...</span></p></div><script>fetch('/api/status').then(r=>r.json()).then(d=>document.getElementById('status').textContent=d.wifi.connected?'Conectado':'Desconectado')</script></body></html>
)rawliteral";
```

### Performance

- ESP32-C3 tiene 4MB Flash → Suficiente para web completa
- RAM: 400KB → Limitar buffers de AsyncWebServer
- Clientes simultáneos: Máx 4-5 para estabilidad

---

## ✅ Checklist de Implementación

- [ ] Instalar librerías necesarias (AsyncWebServer, AsyncTCP)
- [ ] Crear estructura de archivos (web_server.cpp/h, html/)
- [ ] Implementar modo AP con SSID basado en MAC
- [ ] Implementar servidor web en puerto 80
- [ ] Crear página dashboard básica
- [ ] API REST para status (`/api/status`)
- [ ] API REST para configuración (`/api/config`)
- [ ] Páginas de configuración (WiFi, MQTT, Sensores)
- [ ] Integración con EEPROM para persistencia
- [ ] Botón de reinicio funcional
- [ ] Escaneo de redes WiFi
- [ ] Validación de formularios
- [ ] Feedback visual (toast notifications)
- [ ] Responsive design (móvil friendly)
- [ ] Testing en diferentes navegadores
- [ ] Documentación de usuario final

---

## 🎓 Próximos Pasos

1. **Compilar firmware actual** con cambios de configuración Modbus dinámica
2. **Probar comandos MQTT** con diferentes sensores (temp, energía, flujo)
3. **Persistir config en EEPROM** para que sobreviva reinicios
4. **Implementar servidor web** empezando por Fase 1 (básico)
5. **Agregar formularios** para configuración completa (Fases 2-3)
6. **Pulir UI** con gráficos y actualizaciones en vivo (Fase 4)

---

**Versión:** 1.0  
**Fecha:** 19 de Octubre de 2025  
**Autor:** Sistema de Configuración Web ESP32
