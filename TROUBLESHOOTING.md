# Guía de Solución de Problemas

## Problemas de Compilación

### Error: "modbus-esp8266 library not found"

**Solución con PlatformIO:**
```bash
pio pkg install --library "emelianov/modbus-esp8266@^4.1.0"
```

**Solución con Arduino IDE:**
1. Abrir Arduino IDE
2. Ir a `Herramientas` → `Administrar bibliotecas`
3. Buscar "modbus-esp8266"
4. Instalar la biblioteca de "emelianov"

### Error: "ESP32 platform not found"

**Solución con PlatformIO:**
```bash
pio pkg install --platform espressif32
```

**Solución con Arduino IDE:**
1. Ir a `Archivo` → `Preferencias`
2. Agregar URL de gestor de tarjetas ESP32:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Ir a `Herramientas` → `Placa` → `Gestor de tarjetas`
4. Buscar "ESP32" e instalar

### Error de compilación: "config.h not found"

**Causa:** El archivo config.h debe estar en el mismo directorio que el sketch.

**Solución:**
- Para Arduino IDE: Asegurarse de tener `config.h` en la carpeta `Nehuentue_Sensor_ESP32C3/`
- Para PlatformIO: Asegurarse de tener `config.h` en la carpeta `src/`

### Error: "TRegister was not declared"

**Causa:** Versión incorrecta de la biblioteca modbus-esp8266.

**Solución:**
```bash
pio pkg install --library "emelianov/modbus-esp8266@^4.1.0"
```

## Problemas de Comunicación Modbus

### No hay comunicación (no responde a peticiones)

**Verificaciones:**

1. **Conexiones físicas:**
   - Verificar que RX del ESP32 está conectado a RO del MAX485
   - Verificar que TX del ESP32 está conectado a DI del MAX485
   - Verificar que DE/RE está conectado al GPIO10

2. **Alimentación:**
   - Verificar que el MAX485 tiene alimentación (3.3V o 5V según modelo)
   - Verificar que el ESP32-C3 tiene alimentación estable

3. **Configuración:**
   - Verificar que el baudrate coincide (por defecto 9600)
   - Verificar que el Slave ID coincide (por defecto 1)
   - Verificar paridad (por defecto: None)

4. **Bus RS-485:**
   - Verificar polaridad correcta (A+ y B-)
   - Agregar resistencias de terminación de 120Ω en los extremos
   - Verificar que el cable es de par trenzado

### Lecturas erróneas o intermitentes

**Posibles causas:**

1. **Ruido eléctrico:**
   - Usar cable blindado para RS-485
   - Alejar cables de fuentes de ruido (motores, fuentes switching)
   - Conectar malla/blindaje a tierra

2. **Resistencias de terminación:**
   - Agregar 120Ω entre A y B en cada extremo del bus
   - NO agregar en dispositivos intermedios

3. **Longitud del cable:**
   - Máximo 1200m a 9600 baudios
   - Reducir longitud o velocidad si hay problemas

4. **Múltiples dispositivos:**
   - Verificar que cada dispositivo tiene un Slave ID único
   - No exceder 32 dispositivos sin repetidores

### Error CRC

**Causas comunes:**
- Baudrate incorrecto
- Bits de datos incorrectos (debe ser 8)
- Paridad incorrecta
- Ruido en la línea

**Solución:**
1. Verificar configuración con osciloscopio
2. Reducir baudrate (ej: de 19200 a 9600)
3. Mejorar blindaje y tierra
4. Verificar integridad de cables

## Problemas con Sensores

### DHT22 no responde

**Verificaciones:**
1. Conexión correcta (VCC, GND, DATA)
2. Resistencia pull-up de 4.7kΩ entre DATA y VCC
3. Alimentación 3.3V estable
4. Pin correcto en código (GPIO4 por defecto)

**Código de prueba:**
```cpp
#include <DHT.h>
#define DHT_PIN 4
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  
  if (!isnan(temp) && !isnan(hum)) {
    Serial.print("Temp: ");
    Serial.print(temp);
    Serial.print("°C, Hum: ");
    Serial.print(hum);
    Serial.println("%");
  } else {
    Serial.println("Error leyendo DHT22");
  }
  delay(2000);
}
```

### BME280 no se detecta

**Verificaciones:**
1. Dirección I2C correcta (0x76 o 0x77)
2. Conexiones SDA y SCL correctas
3. Resistencias pull-up (algunos módulos las tienen integradas)
4. Alimentación 3.3V

**Código de prueba:**
```cpp
#include <Wire.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  if (!bme.begin(0x76)) {  // Probar también con 0x77
    Serial.println("No se encuentra BME280");
    while (1);
  }
  Serial.println("BME280 iniciado");
}

void loop() {
  Serial.print("Temp: ");
  Serial.print(bme.readTemperature());
  Serial.print("°C, Hum: ");
  Serial.print(bme.readHumidity());
  Serial.print("%, Pres: ");
  Serial.print(bme.readPressure() / 100.0F);
  Serial.println(" hPa");
  delay(2000);
}
```

## Problemas de Hardware

### ESP32-C3 no se detecta por USB

**Soluciones:**
1. **Driver USB-Serial:**
   - Instalar driver CH340 o CP2102 según tu placa
   - Windows: Descargar desde sitio del fabricante
   - Linux: Agregar usuario al grupo dialout: `sudo usermod -a -G dialout $USER`

2. **Modo Boot:**
   - Mantener presionado botón BOOT mientras se conecta USB
   - Soltar después de que se detecte

3. **Cable USB:**
   - Usar cable USB-C con datos (no solo carga)
   - Probar con otro cable

### LED no funciona

**Verificaciones:**
1. Pin correcto (GPIO8 para ESP32-C3-DevKitM-1)
2. LED integrado vs externo
3. Dirección del LED (ánodo/cátodo)
4. Resistencia adecuada (220Ω típico)

### MAX485 no funciona

**Verificaciones:**
1. Alimentación correcta (3.3V o 5V según datasheet)
2. Pin DE/RE conectado y en nivel correcto
3. Dirección de comunicación (DE=HIGH para TX, DE=LOW para RX)
4. Chip no dañado (probar con otro)

## Herramientas de Diagnóstico

### 1. Monitor Serial USB

Habilitar debug en `config.h`:
```cpp
#define DEBUG_ENABLED    1
```

Conectar monitor serial a 115200 baudios para ver mensajes de debug.

### 2. Analizador Lógico

Para analizar señales RS-485:
- Conectar a pines A y B del bus
- Configurar para protocolo UART
- Verificar baudrate, paridad, bits

### 3. Multímetro

Verificar:
- Tensiones de alimentación (3.3V, 5V)
- Continuidad de cables
- Resistencias de pull-up/terminación

### 4. Software Modbus

Para pruebas de comunicación:

**QModMaster (Multiplataforma):**
```bash
# Linux
sudo apt-get install qmodmaster

# Windows/Mac
# Descargar desde sourceforge
```

**Python pymodbus:**
```bash
pip install pymodbus

# Script de prueba
python3 -c "
from pymodbus.client import ModbusSerialClient
client = ModbusSerialClient(port='/dev/ttyUSB0', baudrate=9600)
client.connect()
result = client.read_holding_registers(0, 8, slave=1)
print(result.registers)
client.close()
"
```

## Contacto y Soporte

Si después de seguir esta guía aún tienes problemas:

1. **GitHub Issues:** Abre un issue en el repositorio con:
   - Descripción del problema
   - Configuración de hardware
   - Logs de error
   - Código modificado (si aplica)

2. **Documentación adicional:**
   - [Modbus.org](https://modbus.org/)
   - [ESP32-C3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)
   - [MAX485 Datasheet](https://datasheets.maximintegrated.com/en/ds/MAX1487-MAX491.pdf)

## Checklist de Verificación Rápida

- [ ] Bibliotecas instaladas correctamente
- [ ] Placa ESP32-C3 seleccionada en IDE
- [ ] Conexiones físicas verificadas
- [ ] Alimentación estable (3.3V/5V)
- [ ] Baudrate correcto en maestro y esclavo
- [ ] Slave ID correcto
- [ ] Resistencias de terminación en extremos del bus
- [ ] Cable de par trenzado para RS-485
- [ ] Polaridad correcta (A+, B-)
- [ ] LED parpadea al iniciar
- [ ] Monitor serial muestra mensajes (si debug activado)
