# Guía de Inicio Rápido

Esta guía te ayudará a poner en marcha tu sensor ESP32-C3 Modbus RTU en minutos.

## Paso 1: Requisitos Previos

### Hardware
- [x] ESP32-C3 DevKitM-1 o compatible
- [x] Módulo transceptor RS-485 (MAX485 o similar)
- [x] Cable USB-C
- [x] Cables Dupont para conexiones
- [x] (Opcional) Resistencias de terminación 120Ω

### Software
- [x] Arduino IDE 2.x o PlatformIO
- [x] Driver USB-Serial (CH340/CP2102)

## Paso 2: Instalación del Software

### Opción A: PlatformIO (Recomendado)

```bash
# 1. Instalar PlatformIO
pip install platformio

# 2. Clonar repositorio
git clone https://github.com/Tinex3/Nehuentue_Suit_Sensor_Modbus_RTU.git
cd Nehuentue_Suit_Sensor_Modbus_RTU

# 3. Compilar
pio run

# 4. Subir al ESP32-C3
pio run --target upload

# 5. Ver logs (opcional)
pio device monitor
```

### Opción B: Arduino IDE

```
1. Descargar Arduino IDE desde arduino.cc
2. Agregar soporte ESP32:
   - Archivo → Preferencias
   - URLs Adicionales: 
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
3. Instalar placas ESP32:
   - Herramientas → Gestor de tarjetas → Buscar "ESP32" → Instalar
4. Instalar biblioteca Modbus:
   - Herramientas → Administrar bibliotecas → Buscar "modbus-esp8266" → Instalar
5. Abrir sketch: Nehuentue_Sensor_ESP32C3/Nehuentue_Sensor_ESP32C3.ino
6. Seleccionar placa: ESP32C3 Dev Module
7. Compilar y subir
```

## Paso 3: Conexiones Hardware

### Conexión Mínima (para pruebas)

```
ESP32-C3          MAX485
--------          ------
GPIO20 (RX)  -->  RO
GPIO21 (TX)  -->  DI
GPIO10       -->  DE/RE
GND          -->  GND
3.3V         -->  VCC
```

### Diagrama Visual

```
     +---------+
     | ESP32-C3|
     |         |
     |      20 |----RO----+
     |      21 |----DI----+---- MAX485
     |      10 |----DE/RE-+
     |     GND |----GND---+
     |    3.3V |----VCC---+
     |         |
     | USB-C ⚡|
     +---------+
```

### Conexión al Bus RS-485

```
MAX485         Terminal RS-485
------         ---------------
A          --> A+ (Cable +)
B          --> B- (Cable -)

Nota: Agregar resistencia de 120Ω entre A y B
solo en los extremos del bus
```

## Paso 4: Primera Prueba

### 4.1 Verificar LED de Inicio

Al encender el ESP32-C3, el LED debe parpadear 3 veces rápidamente.
Esto indica que el firmware se cargó correctamente.

### 4.2 Verificar Comunicación con Script

```bash
# Instalar pymodbus
pip install pymodbus

# Ejecutar script de verificación
python3 verify_modbus.py /dev/ttyUSB0 9600 1
```

Deberías ver:
```
✓ Device ID:      1
✓ Firmware:       v1.00
✓ Status:         OK
✓ Temperatura:    25.00 °C
✓ Humedad:        50.00 %
✓ Presión:        1013 hPa
```

### 4.3 Prueba Manual con QModMaster

1. Abrir QModMaster
2. Configurar puerto serial:
   - Puerto: /dev/ttyUSB0 (Linux) o COM# (Windows)
   - Velocidad: 9600
   - Paridad: None
   - Bits de datos: 8
   - Bits de stop: 1
3. Configurar Modbus:
   - Slave ID: 1
   - Función: 03 (Read Holding Registers)
   - Dirección: 0
   - Cantidad: 8
4. Presionar "Send"
5. Deberías ver los valores de los registros

## Paso 5: Configuración (Opcional)

### Cambiar Slave ID

Editar en `config.h`:
```cpp
#define MODBUS_SLAVE_ID  2  // Cambiar de 1 a 2
```

### Cambiar Velocidad (Baudrate)

Editar en `config.h`:
```cpp
#define MODBUS_BAUDRATE  19200  // Cambiar de 9600 a 19200
```

### Habilitar Debug

Editar en `config.h`:
```cpp
#define DEBUG_ENABLED    1  // Cambiar de 0 a 1
```

Luego conectar monitor serial a 115200 baudios para ver logs.

## Paso 6: Integrar Sensores Reales

### Ejemplo: DHT22

1. Conectar DHT22:
   ```
   ESP32-C3    DHT22
   --------    -----
   GPIO4  -->  DATA
   3.3V   -->  VCC
   GND    -->  GND
   ```

2. Agregar resistencia pull-up de 4.7kΩ entre DATA y VCC

3. Modificar código en `Nehuentue_Sensor_ESP32C3.ino`:

```cpp
#include <DHT.h>
#define DHT_PIN 4
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  // ... código existente ...
  dht.begin();
}

void readSensors() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  
  if (!isnan(temp)) {
    holdingRegs[REG_TEMPERATURA] = (uint16_t)(temp * 100);
    mb.Hreg(REG_TEMPERATURA, holdingRegs[REG_TEMPERATURA]);
  }
  
  if (!isnan(hum)) {
    holdingRegs[REG_HUMEDAD] = (uint16_t)(hum * 100);
    mb.Hreg(REG_HUMEDAD, holdingRegs[REG_HUMEDAD]);
  }
}
```

4. Recompilar y subir

## Troubleshooting Rápido

### No compila
- Verificar que está instalada la biblioteca `modbus-esp8266`
- Verificar soporte ESP32 en Arduino IDE

### No sube al ESP32-C3
- Verificar driver USB-Serial instalado
- Presionar botón BOOT al conectar
- Probar con otro cable USB-C

### No hay comunicación Modbus
- Verificar conexiones RX, TX, DE/RE
- Verificar Slave ID (debe coincidir)
- Verificar baudrate (debe coincidir)
- Agregar resistencias de terminación 120Ω

### Lecturas erróneas
- Verificar alimentación estable (3.3V)
- Verificar cable RS-485 de par trenzado
- Reducir longitud de cable o velocidad

## Siguiente Pasos

1. ✓ **Probar comunicación básica** (¡Ya lo hiciste!)
2. → **Integrar sensores reales** (DHT22, BME280, etc.)
3. → **Configurar red Modbus** (agregar más dispositivos)
4. → **Desarrollar aplicación maestra** (Python, Node.js, SCADA)
5. → **Instalación permanente** (PCB, caja, alimentación)

## Recursos Adicionales

- **README.md**: Documentación completa
- **WIRING.md**: Diagramas de conexión detallados
- **EXAMPLES.md**: Ejemplos de código para maestro Modbus
- **TROUBLESHOOTING.md**: Guía completa de solución de problemas
- **ARDUINO_SETUP.md**: Configuración detallada de Arduino IDE

## Ayuda y Soporte

- **Issues**: https://github.com/Tinex3/Nehuentue_Suit_Sensor_Modbus_RTU/issues
- **Documentación Modbus**: https://modbus.org/
- **Foro ESP32**: https://esp32.com/

---

**¿Listo para empezar?** 🚀

Sigue los pasos en orden y en menos de 30 minutos tendrás tu sensor funcionando.
