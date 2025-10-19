# EEPROM Manager - Guía de Uso

## Configuración para diferentes modelos

La librería EEPROM Manager es ultra-genérica y soporta toda la familia 24LCXX.
Simplemente especifica el tamaño en el método `begin()`:

### Modelos soportados

| Modelo   | Tamaño (bytes) | Capacidad | Código de ejemplo |
|----------|----------------|-----------|-------------------|
| 24LC32   | 4096          | 4 KB      | `EEPROM24LC64.begin(8, 9, 4096);` |
| 24LC64   | 8192          | 8 KB      | `EEPROM24LC64.begin(8, 9, 8192);` |
| **24LC128** | **16384**  | **16 KB** | `EEPROM24LC64.begin(8, 9, 16384);` |
| 24LC256  | 32768         | 32 KB     | `EEPROM24LC64.begin(8, 9, 32768);` |
| 24LC512  | 65536         | 64 KB     | `EEPROM24LC64.begin(8, 9, 65536);` |

## Inicialización básica

```cpp
#include "eeprom_manager.h"

void setup() {
    Serial.begin(115200);
    
    // Para 24LC128 (16 KB)
    // Parámetros: SDA, SCL, Tamaño, Frecuencia, Dirección I2C
    EEPROM24LC64.begin(8, 9, 16384);  // 16 KB = 24LC128
    
    // O especifica todo:
    // EEPROM24LC64.begin(8, 9, 16384, 100000, 0x50);
}
```

## Uso con valor por defecto

Si no especificas el tamaño, usa el valor definido en `eeprom_manager.h` (actualmente 16384 = 24LC128):

```cpp
// Usa el tamaño por defecto (16 KB)
EEPROM24LC64.begin(8, 9);
```

## Ejemplos de uso genérico

### 1. Guardar y cargar estructuras personalizadas

```cpp
struct MiConfig {
    int sensor_id;
    float calibration;
    bool enabled;
    char name[32];
};

MiConfig config = {123, 1.5, true, "Sensor Principal"};

// Guardar
EEPROM24LC64.save(0, config);

// Cargar
MiConfig configLeida;
EEPROM24LC64.load(0, configLeida);
```

### 2. Guardar con verificación CRC

```cpp
struct DatosCriticos {
    uint32_t contador;
    float valor_importante;
};

DatosCriticos datos = {12345, 98.6};

// Guardar con CRC (usa 2 bytes extra)
EEPROM24LC64.saveWithCRC(100, datos);

// Cargar y verificar
DatosCriticos datosLeidos;
if (EEPROM24LC64.loadWithCRC(100, datosLeidos) == EEPROM_OK) {
    Serial.println("✓ Datos válidos (CRC correcto)");
} else {
    Serial.println("✗ Datos corruptos (CRC incorrecto)");
}
```

### 3. Guardar arrays

```cpp
float mediciones[10] = {23.5, 24.1, 23.8, ...};

// Guardar array completo
EEPROM24LC64.saveArray(200, mediciones, 10);

// Cargar array
float medidasLeidas[10];
EEPROM24LC64.loadArray(200, medidasLeidas, 10);
```

### 4. Guardar strings

```cpp
String nombreDispositivo = "ESP32-Sensor-001";

// Guardar String (usa longitud + contenido)
EEPROM24LC64.saveString(300, nombreDispositivo);

// Cargar String
String nombreLeido;
EEPROM24LC64.loadString(300, nombreLeido);
```

### 5. Operaciones de utilidad

```cpp
// Borrar rango de memoria
EEPROM24LC64.clear(0, 100);  // Borra 100 bytes desde posición 0

// Borrar toda la EEPROM
EEPROM24LC64.clearAll();

// Llenar con patrón
EEPROM24LC64.fill(0, 100, 0xAA);  // Llena con 0xAA

// Verificar espacio libre
uint16_t libre = EEPROM24LC64.getFreeSpace(500);
Serial.printf("Espacio libre desde 500: %d bytes\n", libre);

// Obtener tamaño total configurado
uint16_t total = EEPROM24LC64.getTotalSize();
Serial.printf("Tamaño total: %d bytes\n", total);

// Ver estado
EEPROM24LC64.printStatus();

// Volcado de memoria (debug)
EEPROM24LC64.dumpMemory(0, 256);  // Muestra primeros 256 bytes
```

## Ejemplo completo con 24LC128

```cpp
#include <Arduino.h>
#include "eeprom_manager.h"

struct SensorConfig {
    uint8_t id;
    float offset_temp;
    float offset_hum;
    uint16_t intervalo_ms;
    char ubicacion[20];
};

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Inicializa EEPROM 24LC128 (16 KB)
    Serial.println("Inicializando EEPROM 24LC128...");
    if (EEPROM24LC64.begin(8, 9, 16384) == EEPROM_OK) {
        Serial.println("✓ EEPROM lista");
    } else {
        Serial.println("✗ Error iniciando EEPROM");
        return;
    }
    
    // Muestra estado
    EEPROM24LC64.printStatus();
    
    // Define configuración
    SensorConfig config = {
        .id = 42,
        .offset_temp = -0.5,
        .offset_hum = 2.3,
        .intervalo_ms = 5000,
        .ubicacion = "Sala Principal"
    };
    
    // Guarda con CRC
    Serial.println("\nGuardando configuración con CRC...");
    if (EEPROM24LC64.saveWithCRC(0, config) == EEPROM_OK) {
        Serial.println("✓ Configuración guardada");
    }
    
    // Simula reinicio - lee configuración
    Serial.println("\nLeyendo configuración...");
    SensorConfig configLeida;
    if (EEPROM24LC64.loadWithCRC(0, configLeida) == EEPROM_OK) {
        Serial.println("✓ Configuración cargada y verificada");
        Serial.printf("  ID: %d\n", configLeida.id);
        Serial.printf("  Offset Temp: %.2f\n", configLeida.offset_temp);
        Serial.printf("  Offset Hum: %.2f\n", configLeida.offset_hum);
        Serial.printf("  Intervalo: %d ms\n", configLeida.intervalo_ms);
        Serial.printf("  Ubicación: %s\n", configLeida.ubicacion);
    } else {
        Serial.println("✗ Error de CRC o lectura");
    }
    
    // Información
    Serial.printf("\nEspacio usado: %d bytes\n", sizeof(SensorConfig) + 2);
    Serial.printf("Espacio libre: %d bytes\n", 
                  EEPROM24LC64.getFreeSpace(sizeof(SensorConfig) + 2));
}

void loop() {
    delay(1000);
}
```

## Cambiar de 24LC64 a 24LC128

**Opción 1:** Cambiar solo el parámetro en `begin()`:

```cpp
// Antes (24LC64 - 8KB):
EEPROM24LC64.begin(8, 9, 8192);

// Ahora (24LC128 - 16KB):
EEPROM24LC64.begin(8, 9, 16384);
```

**Opción 2:** Cambiar el default en `eeprom_manager.h`:

```cpp
// En eeprom_manager.h línea 18:
#define EEPROM_SIZE 16384  // Ya configurado para 24LC128
```

Luego simplemente:
```cpp
EEPROM24LC64.begin(8, 9);  // Usa el default (16KB)
```

## Notas importantes

- **Thread-Safe**: La librería usa mutex, es segura para FreeRTOS
- **CRC16**: Verificación de integridad opcional
- **Page-Aware**: Escrituras automáticas respetando páginas de 32 bytes
- **No Blocking**: Usa driver nativo ESP32 I2C con timeouts
- **Sin límites**: Guarda CUALQUIER estructura con templates
