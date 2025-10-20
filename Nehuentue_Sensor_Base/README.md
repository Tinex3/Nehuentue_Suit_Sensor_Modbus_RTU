# 🌉 Gateway Modbus-MQTT Universal - ESP32-C3

[![ESP32](https://img.shields.io/badge/ESP32--C3-Adafruit_QT_Py-blue)](https://www.adafruit.com/product/5405)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-6.12.0-orange)](https://platformio.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Version](https://img.shields.io/badge/Version-2.0.0-brightgreen)](CHANGELOG.md)
[![Web UI](https://img.shields.io/badge/Web_UI-Implemented-success)](WEB_IMPLEMENTATION.md)

> **Gateway universal** que lee datos de sensores Modbus RTU y los publica vía MQTT, **totalmente reconfigurable sin recompilar**. Incluye **interfaz web completa** para configuración.

---

## 🎯 Características Principales

### ✨ v2.0: Interfaz Web + Configuración Dinámica Total

- ✅ **Interfaz web embebida**: Configura todo desde el navegador
- ✅ **Modo Access Point**: Primera configuración sin WiFi
- ✅ **Sin recompilación necesaria**: Cambios vía MQTT o Web UI
- ✅ **Comandos Modbus configurables**: Función, dirección, registros
- ✅ **Multi-sensor**: Hasta 4 sensores simultáneos
- ✅ **Universal**: Funciona con cualquier sensor Modbus RTU
- ✅ **Conversión flexible**: Fórmula `(raw × multiplier) + offset`

### 🔧 Funcionalidades Técnicas

| Característica | Estado | Descripción |
|---------------|--------|-------------|
| **Modbus RTU** | ✅ | Funciones 0x03, 0x04 (0x01, 0x02 en desarrollo) |
| **MQTT** | ✅ | Publicación con QoS, Last Will, reconexión automática |
| **WiFi Manager** | ✅ | Reconexión automática, múltiples intentos |
| **NTP Sync** | ✅ | Timestamps reales con timezone Chile (UTC-3) |
| **FreeRTOS** | ✅ | 4 tareas paralelas con colas y mutex |
| **Web UI** | ✅ | Interfaz completa con 4 páginas (Dashboard, WiFi, MQTT, Sensores) |
| **API REST** | ✅ | 6 endpoints (status, config, sensors, scan, restart) |
| **Modo AP** | ✅ | Access Point automático para configuración inicial |
| **OTA Updates** | ✅ | AsyncElegantOTA - Actualiza firmware desde navegador |
| **EEPROM** | ⚠️ | Persistencia (en desarrollo) |

---

## 🏗️ Arquitectura

```
┌─────────────────────────────────────────────────────────────────┐
│                      ESP32-C3 (QT Py)                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────┐    ┌──────────────┐    ┌─────────────────┐  │
│  │ modbusTask   │───▶│ decoderTask  │───▶│   mqttTask      │  │
│  │  (Lee RTU)   │    │ (Decodifica) │    │  (Publica)      │  │
│  │              │    │              │    │                 │  │
│  │ • Función 03 │    │ • N registros│    │ • Telemetría    │  │
│  │ • Función 04 │    │ • Fórmulas   │    │ • Estado        │  │
│  │ • Dinámico   │    │ • Multi-sens │    │ • Eventos       │  │
│  └──────────────┘    └──────────────┘    └─────────┬───────┘  │
│         │                    │                      │          │
│         ▼                    ▼                      ▼          │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │              sensorsConfig (RAM)                         │  │
│  │  • modbusFunction, startAddress, registerCount           │  │
│  │  • multiplier, offset, decimals                          │  │
│  │  • type, unit, enabled                                   │  │
│  └──────────────────────────────────────────────────────────┘  │
│                             ▲                                   │
│                             │ (Comandos MQTT)                   │
│                             │                                   │
└─────────────────────────────┼───────────────────────────────────┘
                              │
                    ┌─────────┴─────────┐
                    │ Mosquitto Broker  │
                    │  192.168.1.25     │
                    └───────────────────┘
```

---

## 📦 Hardware

### Componentes

- **MCU**: [Adafruit QT Py ESP32-C3](https://www.adafruit.com/product/5405)
  - CPU: 160 MHz RISC-V
  - RAM: 400 KB
  - Flash: 4 MB
  - WiFi: 2.4 GHz 802.11b/g/n
  
- **RS485 Transceiver**: MAX485 o similar
  - RX (DE/RE): GPIO 20
  - TX (DI): GPIO 21
  
- **Sensor Modbus**: Cualquier sensor/dispositivo compatible con Modbus RTU
  - Baudrate: 9600 bps (configurable)
  - Protocolo: RTU sobre RS485

### Conexiones

```
ESP32-C3 QT Py          MAX485          Sensor Modbus
─────────────          ───────          ─────────────
GPIO 20 (RX)  ─────▶  RO
GPIO 21 (TX)  ─────▶  DI
              
              ─────▶  DE/RE ──┐
                              │
                      A  ─────┼─────▶  A/D+ (Terminal 1)
                      B  ─────┴─────▶  B/D- (Terminal 2)
                      
GND           ─────────────────────▶  GND
```

---

## 🚀 Instalación

### 1. Requisitos

- [PlatformIO](https://platformio.org/) 6.x o superior
- [VS Code](https://code.visualstudio.com/) con extensión PlatformIO
- Broker MQTT (Mosquitto recomendado)

### 2. Clonar Repositorio

```bash
git clone https://github.com/Tinex3/Nehuentue_Suit_Sensor_Modbus_RTU.git
cd Nehuentue_Suit_Sensor_Modbus_RTU/Nehuentue_Sensor_Base
```

### 3. Compilar y Subir

```bash
pio run --target upload
```

### 4. Primera Configuración (Vía Web UI)

1. ESP32 inicia en **modo Access Point**
2. Conecta tu dispositivo a la red WiFi: `Modbus-Config-XXXX`
3. Contraseña: `modbus2024`
4. Abre el navegador en: `http://192.168.4.1`
5. Configura WiFi, MQTT y sensores desde la interfaz web
6. Guarda → ESP32 reinicia y se conecta a tu WiFi

### 5. Uso Normal

- ESP32 se conecta automáticamente a tu WiFi
- Accede a la interfaz web en: `http://[IP_DEL_ESP32]`
- Ver IP en el monitor serial o en tu router

---

## 🌐 Interfaz Web

### Páginas Disponibles

| Página | URL | Descripción |
|--------|-----|-------------|
| **Dashboard** | `/` | Estado del sistema, sensores activos, uptime |
| **WiFi** | `/wifi` | Configurar red WiFi, escanear redes disponibles |
| **MQTT** | `/mqtt` | Configurar broker MQTT, intervalos de publicación |
| **Sensores** | `/sensors` | Configurar hasta 4 sensores con parámetros Modbus |

### Configurar WiFi desde Web

1. Ve a `http://192.168.4.1/wifi` (modo AP) o `http://[IP]/wifi` (modo normal)
2. Click en "🔄 Escanear" para ver redes disponibles
3. Click en una red para autocompletar el SSID
4. Ingresa la contraseña y Device ID
5. Click en "💾 Guardar y Reiniciar"

### Configurar Sensor desde Web

1. Ve a `/sensors`
2. Click en "✏️ Editar" en el sensor que quieras configurar
3. Completa el formulario:
   - **Tipo**: temperature, energy, flow, pressure, etc.
   - **Unidad**: celsius, kWh, m3/h, bar, etc.
   - **Función Modbus**: 0x03 (Holding) o 0x04 (Input)
   - **Dirección Esclavo**: 1-247
   - **Registro Inicial**: 0-65535
   - **Cantidad Registros**: 1-4
   - **Multiplicador**: Para conversión (ej: 0.1 = dividir por 10)
   - **Offset**: Para ajuste de valor
   - **Decimales**: 0-6
4. Click en "💾 Guardar"

---

## 📡 Uso - Configuración MQTT (Alternativa a Web UI)

### Tópicos

```
devices/{deviceId}/telemetry        → Publicación de datos sensores
devices/{deviceId}/status           → Estado del dispositivo
devices/{deviceId}/event/connect    → Evento de conexión
devices/{deviceId}/event/error      → Eventos de error
devices/{deviceId}/cmd/#            → Comandos (suscripción)
```

### Configurar Sensor de Temperatura

```bash
mosquitto_pub -h 192.168.1.25 -t devices/modbus-01/cmd/sensor_config -m '{
  "sensor_id": 0,
  "type": "temperature",
  "unit": "celsius",
  "multiplier": 0.1,
  "offset": 0,
  "decimals": 2,
  "modbus_function": 3,
  "start_address": 0,
  "register_count": 1
}'
```

### Configurar Medidor de Energía

```bash
mosquitto_pub -h 192.168.1.25 -t devices/modbus-01/cmd/sensor_config -m '{
  "sensor_id": 0,
  "type": "energy",
  "unit": "kWh",
  "multiplier": 0.001,
  "offset": 0,
  "decimals": 3,
  "modbus_function": 4,
  "start_address": 4096,
  "register_count": 2
}'
```

### Cambiar Intervalo de Telemetría

```bash
mosquitto_pub -h 192.168.1.25 -t devices/modbus-01/cmd/config -m '{
  "telemetry_interval": 30,
  "status_interval": 300
}'
```

### Reiniciar Dispositivo

```bash
mosquitto_pub -h 192.168.1.25 -t devices/modbus-01/cmd/reboot -m '{"action":"restart"}'
```

---

## 📊 Ejemplo de Payload Telemetría

```json
{
  "device_id": "modbus-01",
  "device_type": "modbus_sensor",
  "timestamp": 1729353245,
  "datetime": "2025-10-19T14:30:45-03:00",
  "sensor": {
    "name": "temperature",
    "type": "temperature",
    "value": 24.5,
    "unit": "celsius",
    "modbus_address": 1,
    "register": 0
  },
  "meta": {
    "uptime": 9432,
    "rssi": -45,
    "firmware": "1.0.0",
    "power_source": "usb"
  }
}
```

---

## 🎨 Tipos de Sensores Soportados

| Tipo | Unidades | Ejemplos |
|------|----------|----------|
| `temperature` | celsius, fahrenheit, kelvin | Termómetros |
| `humidity` | percent | Higrómetros |
| `energy` | kWh, Wh, MWh | Medidores eléctricos |
| `power` | W, kW, MW | Vatímetros |
| `voltage` | V, mV, kV | Voltímetros |
| `current` | A, mA | Amperímetros |
| `flow` | m3/h, L/min, gpm | Flujómetros |
| `pressure` | bar, psi, Pa, kPa | Manómetros |
| `level` | m, cm, mm, % | Sensores de nivel |
| `speed` | rpm, Hz | Tacómetros |
| `weight` | kg, g, ton | Balanzas |
| **CUSTOM** | Cualquiera | Totalmente flexible |

---

## 📚 Documentación

| Archivo | Descripción |
|---------|-------------|
| [`CHANGELOG.md`](CHANGELOG.md) | Historial de cambios |
| [`SENSOR_CONFIG.md`](SENSOR_CONFIG.md) | Guía completa de configuración de sensores |
| [`WEB_CONFIG.md`](WEB_CONFIG.md) | Especificación original de interfaz web |
| [`WEB_IMPLEMENTATION.md`](WEB_IMPLEMENTATION.md) | ✨ Resumen de implementación web completa |
| [`RESUMEN_CAMBIOS_v2.0.md`](RESUMEN_CAMBIOS_v2.0.md) | Resumen de última versión |
| [`MQTT_TOPICS.md`](MQTT_TOPICS.md) | Estructura de tópicos MQTT |
| [`EEPROM_USAGE.md`](EEPROM_USAGE.md) | Uso de memoria EEPROM |
| [`OTA_UPDATES.md`](OTA_UPDATES.md) | ⚡ Guía completa de actualizaciones OTA |

---

## ⚡ Actualizaciones OTA

El firmware soporta **actualizaciones Over-The-Air** sin cables:

1. **Compila el firmware**:
   ```bash
   pio run
   ```

2. **Accede a la interfaz OTA**:
   - Desde Dashboard: Botón **"⚡ OTA Update"**
   - Directo: `http://IP_DEL_ESP32/update`

3. **Sube el firmware**:
   - Archivo: `.pio/build/adafruit_qtpy_esp32c3/firmware.bin`
   - Sube desde navegador
   - Espera barra de progreso 100%
   - ESP32 reinicia automáticamente

📖 **Guía completa**: [`OTA_UPDATES.md`](OTA_UPDATES.md)

---

## 🔧 Desarrollo

### Estructura del Proyecto

```
Nehuentue_Sensor_Base/
├── src/
│   ├── main.cpp              # Punto de entrada
│   ├── tasks.cpp             # Tareas FreeRTOS
│   ├── modbus_rtu.cpp        # Driver Modbus RTU
│   └── eeprom_manager.cpp    # Gestor EEPROM
├── include/
│   ├── tasks.h               # Estructuras y declaraciones
│   ├── modbus_rtu.h          # API Modbus
│   └── eeprom_manager.h      # API EEPROM
├── platformio.ini            # Configuración PlatformIO
├── CHANGELOG.md
├── SENSOR_CONFIG.md
├── WEB_CONFIG.md
└── README.md (este archivo)
```

### Agregar Nuevo Tipo de Sensor

1. Enviar comando MQTT con `type` y `unit` deseados
2. Configurar `multiplier` y `offset` según datasheet del sensor
3. Configurar `modbus_function`, `start_address`, `register_count`
4. ¡Listo! No requiere cambios en código

### Compilar

```bash
pio run
```

### Subir Firmware

```bash
pio run --target upload
```

### Monitor Serial

```bash
pio device monitor -b 115200
```

### Limpiar Build

```bash
pio run --target clean
```

---

## 🐛 Troubleshooting

### Error: "MQTT conectado pero publish falla"

**Solución:** Buffer MQTT muy pequeño. Verificar que `platformio.ini` tenga:
```ini
build_flags = -D MQTT_MAX_PACKET_SIZE=1024
```

### Error: "WiFi no conecta"

**Solución:** 
1. Verificar credenciales en `initDefaultConfig()`
2. Comprobar que red WiFi sea 2.4 GHz (ESP32-C3 no soporta 5 GHz)
3. Ver logs por serial

### Error: "Modbus timeout"

**Solución:**
1. Verificar conexiones RS485 (A/B no invertidos)
2. Comprobar baudrate del sensor (por defecto 9600)
3. Verificar dirección Modbus del esclavo

### Error: "Cola modbusQueue llena"

**Solución:** Reducir intervalo de polling en `modbusTask()`:
```cpp
const TickType_t pollingInterval = pdMS_TO_TICKS(10000); // 10 segundos
```

---

## 🤝 Contribuir

¡Contribuciones son bienvenidas!

1. Fork el proyecto
2. Crea una rama feature (`git checkout -b feature/nueva-funcionalidad`)
3. Commit cambios (`git commit -am 'Agregar nueva funcionalidad'`)
4. Push a la rama (`git push origin feature/nueva-funcionalidad`)
5. Abrir Pull Request

---

## 📝 TODO

- [ ] Persistencia EEPROM de `sensorsConfig`
- [ ] Interfaz web con AsyncWebServer (ver [`WEB_CONFIG.md`](WEB_CONFIG.md))
- [ ] Modo Access Point para configuración inicial
- [ ] Soporte funciones Modbus 0x01, 0x02, 0x05, 0x06
- [ ] OTA Updates
- [ ] Integración con Home Assistant
- [ ] Gráficos en tiempo real (Chart.js)
- [ ] Export/Import configuración JSON
- [ ] Autenticación web
- [ ] HTTPS con certificado autofirmado

---

## 📄 Licencia

MIT License - Ver archivo [LICENSE](LICENSE) para detalles

---

## 👨‍💻 Autor

**Proyecto Nehuentue**  
Suit Sensor Modbus RTU

---

## 🌟 Agradecimientos

- Adafruit por el excelente QT Py ESP32-C3
- Comunidad PlatformIO
- Proyecto Mosquitto MQTT
- Documentación Modbus.org

---

## 📞 Soporte

- 🐛 Issues: [GitHub Issues](https://github.com/Tinex3/Nehuentue_Suit_Sensor_Modbus_RTU/issues)
- 📧 Email: [contacto]
- 💬 Discord: [servidor]

---

**⚡ Gateway Universal. Configuración Dinámica. Sin Recompilaciones. ⚡**
