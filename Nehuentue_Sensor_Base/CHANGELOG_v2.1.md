# 🚀 Nehuentue Sensor v2.1 - Changelog

## Fecha: 20 de octubre de 2025

---

## 📦 **Refactorización Mayor: Eliminación de Servidor Web**

### ❌ **Componentes Eliminados**

1. **AsyncWebServer** - Servidor HTTP asíncrono
2. **AsyncTCP** - Librería TCP asíncrona
3. **AsyncElegantOTA** - OTA por HTTP
4. **web_server.cpp** (927 líneas) - Toda la interfaz web
5. **web_server.h** - Headers del servidor web
6. **Páginas HTML** - Dashboard, WiFi, MQTT, Sensores

### ✅ **Nuevo Sistema**

- **Configuración 100% por MQTT** con comandos JSON
- **Credenciales preconfiguradas** en `config.h`
- **8 comandos MQTT** implementados
- **Sin Task Watchdog issues**
- **Mayor estabilidad**

---

## 📊 **Impacto en Memoria**

| Métrica | Antes (v2.0) | Después (v2.1) | Ahorro |
|---------|--------------|----------------|--------|
| **Flash** | 955,458 bytes (72.9%) | 786,608 bytes (60.0%) | **168,850 bytes (~165 KB)** ✅ |
| **RAM** | 40,932 bytes (12.5%) | 40,340 bytes (12.3%) | **592 bytes** ✅ |
| **Uso Flash** | 72.9% | 60.0% | **-12.9%** ✅ |

**Resultado:** Liberados **165 KB de Flash** y **592 bytes de RAM**.

---

## 🔧 **Credenciales Preconfiguradas**

### WiFi
```cpp
SSID:     "Amanda 2.4G"
Password: "Gomezriquelmegomez12"
Hostname: "Nehuentue-Sensor"
```

### MQTT
```cpp
Server:    "192.168.1.100"
Port:      1883
User:      "mqttuser"
Password:  "1234"
Client ID: "nehuentue_sensor_001"
```

**Nota:** Estas credenciales pueden ser sobrescritas usando comandos MQTT y se guardan en Flash de forma persistente.

---

## 📡 **Comandos MQTT Implementados**

### Tópicos
- **Comandos**: `nehuentue/{clientId}/cmd`
- **Respuestas**: `nehuentue/{clientId}/response`
- **Telemetría**: `nehuentue/{clientId}/telemetry`
- **Estado**: `nehuentue/{clientId}/status`

### Lista de Comandos

1. **`get_status`** - Obtener estado del sistema (uptime, heap, WiFi, MQTT, Modbus)
2. **`get_config`** - Obtener configuración actual (WiFi, MQTT, Sensor)
3. **`set_wifi`** - Configurar WiFi (SSID, password)
4. **`set_mqtt`** - Configurar MQTT (server, port, user, password)
5. **`set_sensor`** - Configurar sensor Modbus (name, address, register, count, multiplier)
6. **`scan_wifi`** - Escanear redes WiFi disponibles
7. **`restart`** - Reiniciar dispositivo
8. **`factory_reset`** - Resetear a valores de fábrica

### Ejemplo de Uso

**Publicar comando:**
```bash
mosquitto_pub -h 192.168.1.100 -u mqttuser -P 1234 \
  -t "nehuentue/nehuentue_sensor_001/cmd" \
  -m '{"cmd":"get_status"}'
```

**Suscribirse a respuestas:**
```bash
mosquitto_sub -h 192.168.1.100 -u mqttuser -P 1234 \
  -t "nehuentue/nehuentue_sensor_001/response"
```

---

## 📝 **Cambios en Archivos**

### Modificados
- ✅ `platformio.ini` - Eliminadas dependencias de servidor web
- ✅ `config.h` - Agregadas credenciales preconfiguradas y defines de tópicos MQTT
- ✅ `main.cpp` - Refactorizado onMqttMessage() con 8 comandos JSON (240 líneas nuevas)
- ✅ `main.cpp` - Setup simplificado sin servidor web

### Renombrados (excluidos de compilación)
- ✅ `src/web_server.cpp` → `src/web_server.cpp.bak`
- ✅ `include/web_server.h` → `include/web_server.h.bak`

### Nuevos
- ✅ `MQTT_COMMANDS.md` - Documentación completa de comandos MQTT
- ✅ `CHANGELOG_v2.1.md` - Este archivo

---

## 🎯 **Ventajas del Nuevo Sistema**

### ✅ **Memoria**
- 165 KB de Flash liberados para futuras funcionalidades
- Menor uso de RAM (menos buffers HTTP)

### ✅ **Estabilidad**
- Sin Task Watchdog issues por escaneo WiFi bloqueante
- Sin manejo de múltiples conexiones HTTP simultáneas
- Menor superficie de ataque (sin servidor HTTP expuesto)

### ✅ **Configuración Remota**
- Control total vía MQTT desde cualquier cliente
- Comandos JSON estructurados y documentados
- Respuestas JSON parseables

### ✅ **IoT Profesional**
- Configuración centralizada en broker MQTT
- Tópicos organizados por cliente
- Telemetría y estado periódicos
- Compatible con Home Assistant, Node-RED, etc.

---

## ⚠️ **Desventajas**

### ❌ **Sin Interfaz Visual**
- No hay página web para configurar
- Requiere cliente MQTT (mosquitto, MQTT Explorer, etc.)
- Menos amigable para usuarios no técnicos

### ❌ **Configuración Inicial**
- Credenciales WiFi y MQTT deben estar preconfiguradas en código
- O configurar vía Serial Monitor si FlashStorage falla

### ❌ **Sin OTA Web**
- Eliminado AsyncElegantOTA
- OTA debe implementarse vía MQTT o HTTP simple en futuro

---

## 🔄 **Migración desde v2.0**

Si tienes v2.0 desplegado:

1. **Actualizar credenciales** en `config.h` antes de compilar
2. **Compilar y subir** firmware v2.1
3. **Verificar conexión WiFi/MQTT** en Serial Monitor (115200 baud)
4. **Probar comandos MQTT** con mosquitto_pub
5. **Configurar credenciales** si son diferentes a las por defecto

---

## 📚 **Documentación Adicional**

- `MQTT_COMMANDS.md` - Guía completa de comandos MQTT
- `README.md` - Documentación general del proyecto
- `MQTT_TOPICS.md` - Estructura de tópicos MQTT
- `config.h` - Configuración de credenciales

---

## 🧪 **Testing**

### Compilación
```bash
✅ Compilación exitosa
✅ Flash: 60.0% (786,608 bytes)
✅ RAM: 12.3% (40,340 bytes)
```

### Próximos Tests
- ⏳ Probar en hardware ESP32-C3
- ⏳ Validar conexión WiFi/MQTT con credenciales preconfiguradas
- ⏳ Probar todos los comandos MQTT
- ⏳ Verificar persistencia de configuración en Flash
- ⏳ Test de estabilidad 24 horas

---

## 🚀 **Próximas Funcionalidades**

1. **Tareas FreeRTOS** - Migrar tasks.cpp para usar managers
2. **Telemetría Automática** - Publicar datos Modbus periódicamente
3. **OTA vía MQTT** - Implementar actualización remota
4. **Dashboard Grafana** - Visualización de datos
5. **Alertas** - Notificaciones por umbrales
6. **Multi-sensor** - Soporte para múltiples dispositivos Modbus

---

## 📞 **Soporte**

Para reportar issues o sugerencias:
- **GitHub**: Nehuentue_Suit_Sensor_Modbus_RTU
- **Serial Debug**: 115200 baud

---

**Versión**: 2.1  
**Fecha**: 20 de octubre de 2025  
**Autor**: Nehuentue  
**Licencia**: MIT
