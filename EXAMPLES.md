# Ejemplos de Uso

## Ejemplo 1: Lectura Básica con Python

```python
#!/usr/bin/env python3
"""
Script de ejemplo para leer sensores del ESP32-C3 via Modbus RTU
Requiere: pip install pymodbus
"""

from pymodbus.client import ModbusSerialClient
import time

def main():
    # Configurar cliente Modbus RTU
    client = ModbusSerialClient(
        port='/dev/ttyUSB0',  # Cambiar según tu sistema
        baudrate=9600,
        parity='N',
        stopbits=1,
        bytesize=8,
        timeout=1
    )
    
    # Conectar
    if not client.connect():
        print("Error: No se pudo conectar al dispositivo")
        return
    
    print("Conectado al sensor ESP32-C3")
    print("-" * 50)
    
    try:
        while True:
            # Leer todos los registros (0-7)
            result = client.read_holding_registers(0, 8, slave=1)
            
            if result.isError():
                print("Error al leer registros")
            else:
                # Decodificar valores
                temperatura = result.registers[0] / 100.0
                humedad = result.registers[1] / 100.0
                presion = result.registers[2]
                device_id = result.registers[3]
                firmware_ver = result.registers[4] / 100.0
                status = result.registers[5]
                led_state = result.registers[6]
                sample_rate = result.registers[7]
                
                # Mostrar valores
                print(f"\nLectura de sensores:")
                print(f"  Temperatura:  {temperatura:.2f} °C")
                print(f"  Humedad:      {humedad:.2f} %")
                print(f"  Presión:      {presion} hPa")
                print(f"  Device ID:    {device_id}")
                print(f"  Firmware:     v{firmware_ver:.2f}")
                print(f"  Estado:       {'OK' if status == 1 else 'Error'}")
                print(f"  LED:          {'ON' if led_state else 'OFF'}")
                print(f"  Sample Rate:  {sample_rate} ms")
            
            time.sleep(2)
    
    except KeyboardInterrupt:
        print("\n\nInterrumpido por usuario")
    
    finally:
        client.close()
        print("Desconectado")

if __name__ == "__main__":
    main()
```

## Ejemplo 2: Control del LED

```python
#!/usr/bin/env python3
"""
Script para controlar el LED del ESP32-C3 via Modbus
"""

from pymodbus.client import ModbusSerialClient
import time
import sys

def control_led(client, state):
    """Controlar LED: 0=OFF, 1=ON"""
    result = client.write_register(6, state, slave=1)
    if result.isError():
        print(f"Error al {'encender' if state else 'apagar'} LED")
        return False
    print(f"LED {'encendido' if state else 'apagado'}")
    return True

def main():
    # Configurar cliente
    client = ModbusSerialClient(
        port='/dev/ttyUSB0',
        baudrate=9600,
        parity='N',
        stopbits=1,
        bytesize=8,
        timeout=1
    )
    
    if not client.connect():
        print("Error de conexión")
        sys.exit(1)
    
    print("Sistema de control LED - ESP32-C3")
    print("Comandos: 1=Encender, 0=Apagar, p=Parpadear, q=Salir")
    
    try:
        while True:
            cmd = input("\nComando: ").lower()
            
            if cmd == '1':
                control_led(client, 1)
            elif cmd == '0':
                control_led(client, 0)
            elif cmd == 'p':
                print("Parpadeando LED...")
                for i in range(5):
                    control_led(client, 1)
                    time.sleep(0.5)
                    control_led(client, 0)
                    time.sleep(0.5)
            elif cmd == 'q':
                break
            else:
                print("Comando no reconocido")
    
    except KeyboardInterrupt:
        print("\n")
    
    finally:
        control_led(client, 0)  # Apagar LED al salir
        client.close()
        print("Desconectado")

if __name__ == "__main__":
    main()
```

## Ejemplo 3: Configurar Tasa de Muestreo

```python
#!/usr/bin/env python3
"""
Script para configurar la tasa de muestreo del sensor
"""

from pymodbus.client import ModbusSerialClient
import sys

def set_sample_rate(client, rate_ms):
    """Configurar tasa de muestreo en milisegundos (100-10000)"""
    if rate_ms < 100 or rate_ms > 10000:
        print("Error: La tasa debe estar entre 100 y 10000 ms")
        return False
    
    result = client.write_register(7, rate_ms, slave=1)
    if result.isError():
        print("Error al configurar tasa de muestreo")
        return False
    
    print(f"Tasa de muestreo configurada a {rate_ms} ms")
    return True

def get_sample_rate(client):
    """Leer tasa de muestreo actual"""
    result = client.read_holding_registers(7, 1, slave=1)
    if result.isError():
        print("Error al leer tasa de muestreo")
        return None
    return result.registers[0]

def main():
    if len(sys.argv) < 2:
        print("Uso: python set_sample_rate.py <tasa_en_ms>")
        print("Ejemplo: python set_sample_rate.py 2000")
        sys.exit(1)
    
    try:
        rate = int(sys.argv[1])
    except ValueError:
        print("Error: La tasa debe ser un número entero")
        sys.exit(1)
    
    # Conectar
    client = ModbusSerialClient(
        port='/dev/ttyUSB0',
        baudrate=9600,
        parity='N',
        stopbits=1,
        bytesize=8,
        timeout=1
    )
    
    if not client.connect():
        print("Error de conexión")
        sys.exit(1)
    
    # Leer tasa actual
    current = get_sample_rate(client)
    if current:
        print(f"Tasa actual: {current} ms")
    
    # Configurar nueva tasa
    if set_sample_rate(client, rate):
        # Verificar
        new_rate = get_sample_rate(client)
        if new_rate:
            print(f"Tasa verificada: {new_rate} ms")
    
    client.close()

if __name__ == "__main__":
    main()
```

## Ejemplo 4: Monitoreo Continuo con Registro CSV

```python
#!/usr/bin/env python3
"""
Script para monitoreo continuo y guardado en CSV
"""

from pymodbus.client import ModbusSerialClient
import time
import csv
from datetime import datetime

def main():
    # Configurar cliente
    client = ModbusSerialClient(
        port='/dev/ttyUSB0',
        baudrate=9600,
        parity='N',
        stopbits=1,
        bytesize=8,
        timeout=1
    )
    
    if not client.connect():
        print("Error de conexión")
        return
    
    # Crear archivo CSV
    filename = f"sensor_log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
    
    with open(filename, 'w', newline='') as csvfile:
        fieldnames = ['timestamp', 'temperatura', 'humedad', 'presion', 'status']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        
        print(f"Registrando datos en {filename}")
        print("Presiona Ctrl+C para detener")
        
        try:
            while True:
                # Leer sensores
                result = client.read_holding_registers(0, 6, slave=1)
                
                if not result.isError():
                    timestamp = datetime.now().isoformat()
                    temperatura = result.registers[0] / 100.0
                    humedad = result.registers[1] / 100.0
                    presion = result.registers[2]
                    status = result.registers[5]
                    
                    # Escribir en CSV
                    writer.writerow({
                        'timestamp': timestamp,
                        'temperatura': temperatura,
                        'humedad': humedad,
                        'presion': presion,
                        'status': status
                    })
                    csvfile.flush()
                    
                    # Mostrar en consola
                    print(f"{timestamp} - T:{temperatura:.2f}°C H:{humedad:.2f}% P:{presion}hPa")
                
                time.sleep(5)  # Log cada 5 segundos
        
        except KeyboardInterrupt:
            print(f"\n\nDatos guardados en {filename}")
    
    client.close()

if __name__ == "__main__":
    main()
```

## Ejemplo 5: Cliente Modbus RTU en Node.js

```javascript
// Requiere: npm install modbus-serial

const ModbusRTU = require("modbus-serial");
const client = new ModbusRTU();

// Configuración
const config = {
    port: "/dev/ttyUSB0",  // Puerto serial
    baudRate: 9600,
    dataBits: 8,
    stopBits: 1,
    parity: "none",
    slaveId: 1
};

async function connectAndRead() {
    try {
        // Conectar
        await client.connectRTUBuffered(config.port, {
            baudRate: config.baudRate
        });
        
        client.setID(config.slaveId);
        client.setTimeout(1000);
        
        console.log("Conectado al sensor ESP32-C3");
        
        // Leer sensores cada 2 segundos
        setInterval(async () => {
            try {
                // Leer registros 0-7
                const data = await client.readHoldingRegisters(0, 8);
                
                const temperatura = data.data[0] / 100.0;
                const humedad = data.data[1] / 100.0;
                const presion = data.data[2];
                const deviceId = data.data[3];
                const firmware = data.data[4] / 100.0;
                const status = data.data[5];
                const led = data.data[6];
                const sampleRate = data.data[7];
                
                console.log("\n--- Lectura de Sensores ---");
                console.log(`Temperatura:  ${temperatura.toFixed(2)} °C`);
                console.log(`Humedad:      ${humedad.toFixed(2)} %`);
                console.log(`Presión:      ${presion} hPa`);
                console.log(`Device ID:    ${deviceId}`);
                console.log(`Firmware:     v${firmware.toFixed(2)}`);
                console.log(`Estado:       ${status === 1 ? 'OK' : 'Error'}`);
                console.log(`LED:          ${led ? 'ON' : 'OFF'}`);
                console.log(`Sample Rate:  ${sampleRate} ms`);
                
            } catch (err) {
                console.error("Error al leer:", err.message);
            }
        }, 2000);
        
    } catch (err) {
        console.error("Error de conexión:", err.message);
        process.exit(1);
    }
}

// Manejar cierre graceful
process.on('SIGINT', () => {
    console.log("\nCerrando conexión...");
    client.close(() => {
        console.log("Desconectado");
        process.exit(0);
    });
});

connectAndRead();
```

## Notas

- Cambiar `/dev/ttyUSB0` por el puerto correcto en tu sistema
- En Windows usar `COM1`, `COM2`, etc.
- Instalar dependencias Python: `pip install pymodbus`
- Instalar dependencias Node.js: `npm install modbus-serial`
