# Nehuentue Suit Sensor - Modbus RTU para ESP32-C3

Firmware de Arduino para sensor basado en ESP32-C3 con comunicación Modbus RTU.

## Descripción

Este proyecto implementa un sensor inteligente utilizando el microcontrolador ESP32-C3 que se comunica mediante el protocolo Modbus RTU sobre RS-485. Es ideal para aplicaciones industriales, domótica y sistemas de monitoreo.

## Características

- ✅ Compatible con ESP32-C3 (DevKitM-1, DevKitC-02)
- ✅ Comunicación Modbus RTU esclavo
- ✅ Interfaz RS-485 para comunicación industrial
- ✅ Lectura de múltiples sensores (temperatura, humedad, presión)
- ✅ Configuración mediante registros Modbus
- ✅ Control remoto del LED de estado
- ✅ Tasa de muestreo ajustable
- ✅ Framework Arduino para fácil desarrollo

## Hardware Requerido

### Componentes Principales
- **Microcontrolador**: ESP32-C3-DevKitM-1 o compatible
- **Transceptor RS-485**: MAX485, MAX3485 o similar
- **Sensores** (opcional): DHT22, BME280, DS18B20, etc.
- **Fuente de alimentación**: 5V USB o externa

### Diagrama de Conexiones

```
ESP32-C3          MAX485/RS-485
---------         -------------
GPIO20 (RX) ----> RO (Receiver Output)
GPIO21 (TX) ----> DI (Driver Input)
GPIO10      ----> DE/RE (Direction Control)
GND         ----> GND
3.3V        ----> VCC

MAX485            Bus RS-485
---------         -----------
A             ----> A (Terminal +)
B             ----> B (Terminal -)
```

### Pinout ESP32-C3

| Pin GPIO | Función         | Descripción                    |
|----------|-----------------|--------------------------------|
| GPIO20   | MODBUS_RX       | Recepción UART1                |
| GPIO21   | MODBUS_TX       | Transmisión UART1              |
| GPIO10   | DE/RE           | Control dirección RS-485       |
| GPIO8    | LED             | LED de estado (integrado)      |

## Instalación

### Opción 1: PlatformIO (Recomendado)

1. Instalar [PlatformIO](https://platformio.org/)
2. Clonar este repositorio:
   ```bash
   git clone https://github.com/Tinex3/Nehuentue_Suit_Sensor_Modbus_RTU.git
   cd Nehuentue_Suit_Sensor_Modbus_RTU
   ```
3. Compilar el proyecto:
   ```bash
   pio run
   ```
4. Subir al ESP32-C3:
   ```bash
   pio run --target upload
   ```

### Opción 2: Arduino IDE

1. Instalar [Arduino IDE](https://www.arduino.cc/en/software)
2. Agregar soporte para ESP32:
   - Ir a `Archivo` → `Preferencias`
   - En "URLs Adicionales de Gestor de Tarjetas" agregar:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
3. Instalar placas ESP32:
   - `Herramientas` → `Placa` → `Gestor de tarjetas`
   - Buscar "ESP32" e instalar
4. Instalar biblioteca Modbus:
   - `Herramientas` → `Administrar bibliotecas`
   - Buscar "modbus-esp8266" por emelianov e instalar
5. Abrir `Nehuentue_Sensor_ESP32C3/Nehuentue_Sensor_ESP32C3.ino`
6. Seleccionar placa: `Herramientas` → `Placa` → `ESP32C3 Dev Module`
7. Compilar y subir

## Configuración

### Configuración Modbus

Por defecto el firmware está configurado con:
- **ID Esclavo**: 1
- **Baudrate**: 9600 bps
- **Configuración**: 8N1 (8 bits de datos, sin paridad, 1 bit de parada)

Para cambiar estos valores, editar en el archivo `.ino`:
```cpp
#define MODBUS_SLAVE_ID  1
#define MODBUS_BAUDRATE  9600
#define MODBUS_CONFIG    SERIAL_8N1
```

## Registros Modbus

### Holding Registers (Función 03 - Lectura, Función 06/16 - Escritura)

| Dirección | Nombre          | Tipo      | Descripción                              | Unidad    |
|-----------|-----------------|-----------|------------------------------------------|-----------|
| 0         | TEMPERATURA     | R         | Temperatura × 100                        | °C × 100  |
| 1         | HUMEDAD         | R         | Humedad relativa × 100                   | % × 100   |
| 2         | PRESION         | R         | Presión atmosférica                      | hPa       |
| 3         | DEVICE_ID       | R         | ID del dispositivo                       | -         |
| 4         | FIRMWARE_VER    | R         | Versión del firmware × 100               | -         |
| 5         | STATUS          | R         | Estado del sensor (1=OK, 0=Error)        | -         |
| 6         | LED_CONTROL     | R/W       | Control del LED (0=OFF, 1=ON)            | -         |
| 7         | SAMPLE_RATE     | R/W       | Intervalo de muestreo                    | ms        |

**Nota**: R = Solo lectura, R/W = Lectura y escritura

### Ejemplos de Lectura

**Ejemplo 1**: Leer temperatura (registro 0)
```
Petición:  01 03 00 00 00 01 [CRC]
Respuesta: 01 03 02 09 C4 [CRC]
Valor: 0x09C4 = 2500 → 25.00°C
```

**Ejemplo 2**: Leer todos los registros (0-7)
```
Petición:  01 03 00 00 00 08 [CRC]
Respuesta: 01 03 10 [16 bytes de datos] [CRC]
```

### Ejemplos de Escritura

**Ejemplo 1**: Encender LED (registro 6)
```
Petición:  01 06 00 06 00 01 [CRC]
Respuesta: 01 06 00 06 00 01 [CRC]
```

**Ejemplo 2**: Cambiar intervalo de muestreo a 2000ms (registro 7)
```
Petición:  01 06 00 07 07 D0 [CRC]
Respuesta: 01 06 00 07 07 D0 [CRC]
Valor: 0x07D0 = 2000ms
```

## Integración de Sensores

El firmware viene con sensores simulados. Para integrar sensores reales, modificar la función `readSensors()`:

### Ejemplo con DHT22

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
  
  if (!isnan(temp) && !isnan(hum)) {
    holdingRegs[REG_TEMPERATURA] = (uint16_t)(temp * 100);
    holdingRegs[REG_HUMEDAD] = (uint16_t)(hum * 100);
    mb.Hreg(REG_TEMPERATURA, holdingRegs[REG_TEMPERATURA]);
    mb.Hreg(REG_HUMEDAD, holdingRegs[REG_HUMEDAD]);
  }
}
```

### Ejemplo con BME280

```cpp
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;

void setup() {
  // ... código existente ...
  bme.begin(0x76);
}

void readSensors() {
  float temp = bme.readTemperature();
  float hum = bme.readHumidity();
  float pres = bme.readPressure() / 100.0F;
  
  holdingRegs[REG_TEMPERATURA] = (uint16_t)(temp * 100);
  holdingRegs[REG_HUMEDAD] = (uint16_t)(hum * 100);
  holdingRegs[REG_PRESION] = (uint16_t)pres;
  
  mb.Hreg(REG_TEMPERATURA, holdingRegs[REG_TEMPERATURA]);
  mb.Hreg(REG_HUMEDAD, holdingRegs[REG_HUMEDAD]);
  mb.Hreg(REG_PRESION, holdingRegs[REG_PRESION]);
}
```

## Pruebas

### Script de Verificación Automática

Se incluye un script Python para verificar automáticamente la comunicación:

```bash
# Instalar dependencia
pip install pymodbus

# Ejecutar verificación
python3 verify_modbus.py /dev/ttyUSB0 9600 1

# En Windows
python verify_modbus.py COM3 9600 1
```

El script verificará:
- ✓ Conexión serial
- ✓ Comunicación Modbus
- ✓ Lectura de información del dispositivo
- ✓ Lectura de sensores
- ✓ Control del LED
- ✓ Configuración de parámetros

### Usando QModMaster (Linux/Windows/Mac)

1. Descargar [QModMaster](https://sourceforge.net/projects/qmodmaster/)
2. Conectar el módulo RS-485 al PC
3. Configurar:
   - Puerto serie: /dev/ttyUSB0 (Linux) o COM# (Windows)
   - Baudrate: 9600
   - Paridad: None
   - Bits de datos: 8
   - Bits de parada: 1
   - Slave ID: 1
4. Leer registros usando función 03 (Read Holding Registers)

### Usando ModbusPoll (Windows)

1. Descargar [Modbus Poll](https://www.modbustools.com/modbus_poll.html)
2. Configurar conexión RTU
3. Leer/escribir registros según la tabla

### Usando Python con pymodbus

```python
from pymodbus.client import ModbusSerialClient

# Configurar cliente
client = ModbusSerialClient(
    port='/dev/ttyUSB0',
    baudrate=9600,
    parity='N',
    stopbits=1,
    bytesize=8,
    timeout=1
)

# Conectar
client.connect()

# Leer temperatura (registro 0)
result = client.read_holding_registers(0, 1, slave=1)
temp = result.registers[0] / 100.0
print(f"Temperatura: {temp}°C")

# Encender LED (registro 6)
client.write_register(6, 1, slave=1)

# Cerrar conexión
client.close()
```

## Troubleshooting

### El firmware no compila
- Verificar que está instalada la biblioteca `modbus-esp8266`
- Asegurar que tiene instalado el soporte para ESP32
- Revisar que la versión de Arduino o PlatformIO esté actualizada

### No hay comunicación Modbus
- Verificar conexiones del transceptor RS-485
- Comprobar que el baudrate coincide en maestro y esclavo
- Verificar que el pin DE/RE está conectado y configurado
- Revisar la polaridad de A y B del bus RS-485
- Usar un osciloscopio o analizador lógico para verificar señales

### Lecturas erróneas
- Verificar que los sensores están correctamente conectados
- Comprobar la alimentación de los sensores (3.3V o 5V según modelo)
- Revisar las resistencias pull-up en I2C si corresponde
- Validar que los valores se multiplican por 100 correctamente

## Mejoras Futuras

- [ ] Soporte para múltiples sensores I2C
- [ ] Modo maestro Modbus
- [ ] Almacenamiento de configuración en NVS
- [ ] Web server para configuración
- [ ] Soporte MQTT en paralelo
- [ ] OTA (Over-The-Air updates)
- [ ] Logs de diagnóstico

## Contribuciones

Las contribuciones son bienvenidas. Por favor:
1. Fork del repositorio
2. Crear una rama para tu feature
3. Commit de los cambios
4. Push a la rama
5. Crear un Pull Request

## Licencia

Este proyecto es de código abierto. Ver el archivo LICENSE para más detalles.

## Autor

**Tinex3**

## Referencias

- [Documentación ESP32-C3](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/)
- [Arduino ESP32](https://github.com/espressif/arduino-esp32)
- [Modbus-ESP8266](https://github.com/emelianov/modbus-esp8266)
- [Especificación Modbus RTU](https://modbus.org/docs/Modbus_over_serial_line_V1_02.pdf)

---

**Versión**: 1.0.0  
**Última actualización**: 2025