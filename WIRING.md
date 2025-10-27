# Diagrama de Conexiones - ESP32-C3 con RS-485

## Conexión Básica ESP32-C3 + MAX485

```
                                    RS-485 Bus
                                    Terminal Block
                                    +----+----+
                                    | A  | B  |
                                    +----+----+
                                       |    |
                                       |    |
         ESP32-C3                 MAX485 Module
    +--------------+           +----------------+
    |              |           |                |
    |   GPIO20 (RX)|---------->| RO             |
    |   GPIO21 (TX)|<----------| DI             |
    |   GPIO10     |---------->| DE/RE          |
    |              |           |                |
    |   GND        |---------->| GND            |
    |   3.3V       |---------->| VCC            |
    |              |           |                |
    |   GPIO8 (LED)|--[220Ω]-->LED-->|GND       |
    |              |           |                |
    |   USB-C      |           | A  |---------->| A+ (Terminal)
    |              |           | B  |---------->| B- (Terminal)
    +--------------+           +----------------+

    Notas:
    - La resistencia de 220Ω es opcional si usas el LED interno
    - VCC del MAX485 puede ser 3.3V o 5V según el modelo
    - Terminación de 120Ω entre A y B en los extremos del bus
```

## Conexión con Sensor DHT22

```
         ESP32-C3                  DHT22
    +--------------+           +---------+
    |              |           |         |
    |   GPIO4      |---------->| DATA    |
    |              |           |         |
    |   3.3V       |---------->| VCC     |
    |   GND        |---------->| GND     |
    |              |           |         |
    +--------------+           +---------+
                                    |
                                [4.7kΩ]
                                    |
                                  3.3V

    Nota: Resistencia pull-up de 4.7kΩ entre DATA y VCC
```

## Conexión con Sensor BME280 (I2C)

```
         ESP32-C3                  BME280
    +--------------+           +---------+
    |              |           |         |
    |   GPIO6 (SDA)|<--------->| SDA     |
    |   GPIO7 (SCL)|---------->| SCL     |
    |              |           |         |
    |   3.3V       |---------->| VCC     |
    |   GND        |---------->| GND     |
    |              |           |         |
    +--------------+           +---------+

    Nota: Algunos módulos BME280 ya tienen resistencias pull-up integradas
```

## Conexión Completa (MAX485 + DHT22 + BME280)

```
                                                    RS-485 Bus
                                                    +---------+
                                                    | A  | B  |
                                                    +----+----+
                                                       |    |
         ESP32-C3                                      |    |
    +--------------+          MAX485              +----+----+
    |              |      +----------+            |         |
    |   GPIO20 (RX)|----->| RO       |            |  120Ω   | (Terminación)
    |   GPIO21 (TX)|<-----| DI       |            |         |
    |   GPIO10     |----->| DE/RE    |            +---------+
    |              |      |          |
    |   GPIO4      |---+  | A   |----+---> A+ (Terminal)
    |              |   |  | B   |--------> B- (Terminal)
    |   GPIO6 (SDA)|---+--| VCC      |
    |   GPIO7 (SCL)|   |  | GND      |
    |              |   |  +----------+
    |   GND        |---+---------------+
    |   3.3V       |---+------+        |
    |              |   |      |        |
    +--------------+   |   DHT22    BME280
                       |   +---+    +-----+
                       +-->|VCC|    |VCC  |
                       +-->|GND|    |GND  |
                       |   |DAT|--->|SDA  |
                       |   +---+    |SCL  |
                       |      |     +-----+
                       |   [4.7kΩ]
                       +------+
```

## Pinout Detallado ESP32-C3-DevKitM-1

```
                    ESP32-C3-DevKitM-1
                   +-------------------+
               GND |  1             40 | GND
        (NC)   3V3 |  2             39 | GPIO0 (Boot)
        (NC)    EN |  3             38 | GPIO1
  (ADC1_CH0) GPIO4 |  4             37 | GPIO10 (DE/RE)
  (ADC1_CH1) GPIO5 |  5             36 | GND
  (ADC1_CH2) GPIO6 |  6             35 | 3V3
  (ADC1_CH3) GPIO7 |  7             34 | GPIO18
              GPIO8|  8 (LED)       33 | GPIO19
              GPIO9|  9             32 | GPIO20 (RX1)
             GPIO2| 10             31 | GPIO21 (TX1)
             GPIO3| 11             30 | GND
               GND| 12             29 | 5V
               GND| 13             28 | 5V
               +5V| 14             27 | (NC)
               +5V| 15             26 | (NC)
               (NC)| 16             25 | (NC)
               (NC)| 17             24 | (NC)
               (NC)| 18             23 | (NC)
               (NC)| 19             22 | (NC)
               GND| 20             21 | GND
                   +-------------------+
                         USB-C

Pines Utilizados:
- GPIO20: RX1 (UART1 - Modbus RX)
- GPIO21: TX1 (UART1 - Modbus TX)
- GPIO10: Control DE/RE (RS-485)
- GPIO8:  LED de estado (integrado)
- GPIO4:  Sensor DHT22 (opcional)
- GPIO6:  SDA I2C (BME280, opcional)
- GPIO7:  SCL I2C (BME280, opcional)
```

## Lista de Materiales (BOM)

### Componentes Obligatorios
1. **ESP32-C3-DevKitM-1** o compatible
2. **Módulo MAX485** o MAX3485 (transceptor RS-485)
3. **Cable USB-C** (para programación y alimentación)
4. **Cables Dupont** (macho-macho y macho-hembra)
5. **Resistencias de terminación** 120Ω (2 piezas, para extremos del bus)

### Componentes Opcionales
6. **Sensor DHT22** (temperatura y humedad)
7. **Sensor BME280** (temperatura, humedad y presión)
8. **Sensor DS18B20** (temperatura)
9. **Protoboard** (para prototipos)
10. **Fuente 5V** (si se alimenta externamente)
11. **LED + Resistencia 220Ω** (si no se usa LED integrado)
12. **Resistencia 4.7kΩ** (pull-up para DHT22/DS18B20)

### Materiales para Instalación Permanente
13. **PCB** o placa perforada
14. **Conectores de tornillo** para RS-485
15. **Caja/gabinete** (protección)
16. **Cable de par trenzado** (para bus RS-485)
17. **Espaciadores** y tornillos M3

## Notas Importantes

### Alimentación
- El ESP32-C3 funciona con 3.3V lógica
- Puede alimentarse por USB-C (5V) o pin 5V
- Consumo típico: 50-100mA (sin WiFi)
- Consumo con WiFi: hasta 350mA

### RS-485
- Usar cable de par trenzado para distancias mayores a 1 metro
- Longitud máxima del bus: 1200 metros
- Número máximo de dispositivos: 32 (con repetidores: 256)
- Polaridad importante: A(+) y B(-)
- Resistencias de terminación solo en los extremos del bus

### Sensores
- DHT22: 3.3V, interfaz de un solo cable
- BME280: 3.3V, interfaz I2C o SPI
- DS18B20: 3.3V, interfaz OneWire
- Verificar voltaje de operación antes de conectar

### Protección
- Usar diodos TVS para protección contra sobretensión en RS-485
- Considerar optoacopladores para aislamiento galvánico
- Fusible en la alimentación para protección contra cortocircuitos
