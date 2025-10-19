# Configuración para Arduino IDE

## Pasos de Instalación

1. **Instalar Arduino IDE**
   - Descargar desde: https://www.arduino.cc/en/software
   - Versión mínima recomendada: 2.0.0

2. **Agregar Soporte ESP32**
   - Abrir Arduino IDE
   - Ir a `Archivo` → `Preferencias`
   - En "URLs Adicionales de Gestor de Tarjetas" agregar:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Clic en "OK"

3. **Instalar Placas ESP32**
   - Ir a `Herramientas` → `Placa` → `Gestor de tarjetas`
   - Buscar "ESP32"
   - Instalar "esp32 by Espressif Systems"
   - Versión recomendada: 2.0.11 o superior

4. **Instalar Bibliotecas**
   - Ir a `Herramientas` → `Administrar bibliotecas`
   - Buscar e instalar:
     - `modbus-esp8266` por emelianov (versión 4.1.0 o superior)

5. **Configurar Placa**
   - `Herramientas` → `Placa` → `ESP32 Arduino` → `ESP32C3 Dev Module`
   
   Configuraciones específicas:
   - USB CDC On Boot: "Enabled"
   - CPU Frequency: "160MHz (WiFi)"
   - Flash Frequency: "80MHz"
   - Flash Mode: "QIO"
   - Flash Size: "4MB (32Mb)"
   - Partition Scheme: "Default 4MB with spiffs"
   - Upload Speed: "921600"

6. **Seleccionar Puerto**
   - Conectar ESP32-C3 por USB
   - `Herramientas` → `Puerto` → Seleccionar puerto COM (Windows) o /dev/ttyUSB# (Linux)

7. **Compilar y Subir**
   - Abrir `Nehuentue_Sensor_ESP32C3/Nehuentue_Sensor_ESP32C3.ino`
   - Clic en "Verificar" para compilar
   - Clic en "Subir" para cargar al ESP32-C3

## Notas Importantes

- Si no se detecta el puerto, instalar driver CH340/CP2102 según el chip USB-Serial de tu placa
- En Linux puede ser necesario agregar permisos: `sudo usermod -a -G dialout $USER`
- Si hay error al subir, mantener presionado el botón BOOT mientras se conecta
