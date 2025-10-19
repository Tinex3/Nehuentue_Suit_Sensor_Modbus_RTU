# Changelog

Todos los cambios notables en este proyecto serán documentados en este archivo.

El formato está basado en [Keep a Changelog](https://keepachangelog.com/es-ES/1.0.0/),
y este proyecto adhiere a [Semantic Versioning](https://semver.org/lang/es/).

## [1.0.0] - 2025-10-19

### Agregado
- Firmware inicial para ESP32-C3 con soporte Modbus RTU
- Implementación de esclavo Modbus RTU sobre RS-485
- Soporte para 8 registros Holding (temperatura, humedad, presión, etc.)
- Control remoto de LED vía Modbus
- Configuración de tasa de muestreo vía Modbus
- Sensores simulados para pruebas
- Archivo de configuración `config.h` para fácil personalización
- Soporte para PlatformIO y Arduino IDE
- Documentación completa en español:
  - README.md - Documentación principal
  - QUICKSTART.md - Guía de inicio rápido
  - WIRING.md - Diagramas de conexiones
  - EXAMPLES.md - Ejemplos de uso con Python y Node.js
  - ARDUINO_SETUP.md - Configuración de Arduino IDE
  - TROUBLESHOOTING.md - Guía de solución de problemas
- Script de verificación automática (verify_modbus.py)
- Ejemplos de cliente Modbus en Python
- Ejemplos de cliente Modbus en Node.js
- Licencia MIT
- .gitignore para archivos de compilación

### Características
- Compatible con ESP32-C3-DevKitM-1 y ESP32-C3-DevKitC-02
- Comunicación Modbus RTU a 9600 baudios (configurable)
- 8 registros Holding disponibles
- Control de LED integrado
- Tasa de muestreo configurable (100ms - 10s)
- Soporte para debug por Serial USB
- Callbacks para escritura de registros
- Validación de rangos en configuración

### Registros Modbus
- Registro 0: Temperatura (x100)
- Registro 1: Humedad (x100)
- Registro 2: Presión (hPa)
- Registro 3: Device ID (solo lectura)
- Registro 4: Versión de firmware (solo lectura)
- Registro 5: Estado del sensor (solo lectura)
- Registro 6: Control de LED (lectura/escritura)
- Registro 7: Tasa de muestreo (lectura/escritura)

### Pines utilizados
- GPIO20: RX (UART1) - Modbus
- GPIO21: TX (UART1) - Modbus
- GPIO10: DE/RE - Control de dirección RS-485
- GPIO8: LED de estado

### Dependencias
- Arduino Framework para ESP32
- Biblioteca modbus-esp8266 v4.1.0+
- Platform espressif32

## [Unreleased]

### Planeado
- Soporte para sensores reales (DHT22, BME280, DS18B20)
- Almacenamiento de configuración en NVS (memoria no volátil)
- Modo maestro Modbus
- Web server para configuración
- Soporte MQTT en paralelo
- OTA (actualización Over-The-Air)
- Logs de diagnóstico
- Soporte para múltiples sensores I2C
- Estadísticas de comunicación Modbus
- Watchdog timer configurable

---

## Formato de Versiones

El proyecto usa [Semantic Versioning](https://semver.org/):
- **MAJOR**: Cambios incompatibles en la API
- **MINOR**: Nuevas funcionalidades compatibles
- **PATCH**: Correcciones de bugs compatibles

## Tipos de Cambios

- **Agregado**: Para nuevas funcionalidades
- **Cambiado**: Para cambios en funcionalidades existentes
- **Obsoleto**: Para funcionalidades que serán eliminadas
- **Eliminado**: Para funcionalidades eliminadas
- **Corregido**: Para correcciones de bugs
- **Seguridad**: Para vulnerabilidades de seguridad
