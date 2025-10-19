#!/usr/bin/env python3
"""
Script de verificación para ESP32-C3 Modbus RTU
Verifica la comunicación y muestra información del dispositivo

Uso: python3 verify_modbus.py [puerto] [baudrate] [slave_id]
Ejemplo: python3 verify_modbus.py /dev/ttyUSB0 9600 1
"""

import sys
import time

try:
    from pymodbus.client import ModbusSerialClient
except ImportError:
    print("Error: pymodbus no está instalado")
    print("Instalar con: pip install pymodbus")
    sys.exit(1)

def print_header():
    print("=" * 60)
    print("  Verificación de Nehuentue Sensor ESP32-C3 Modbus RTU")
    print("=" * 60)
    print()

def test_connection(port, baudrate, slave_id):
    """Probar conexión básica al dispositivo"""
    print(f"Configuración:")
    print(f"  Puerto:   {port}")
    print(f"  Baudrate: {baudrate}")
    print(f"  Slave ID: {slave_id}")
    print()
    
    try:
        client = ModbusSerialClient(
            port=port,
            baudrate=baudrate,
            parity='N',
            stopbits=1,
            bytesize=8,
            timeout=2
        )
        
        if not client.connect():
            print("❌ Error: No se pudo abrir el puerto serial")
            print(f"   Verificar que {port} existe y tiene permisos")
            return None
        
        print("✓ Puerto serial abierto correctamente")
        return client
    
    except Exception as e:
        print(f"❌ Error al conectar: {e}")
        return None

def read_device_info(client, slave_id):
    """Leer información del dispositivo"""
    print("\nLeyendo información del dispositivo...")
    print("-" * 60)
    
    try:
        # Leer registros 3, 4, 5 (Device ID, Firmware, Status)
        result = client.read_holding_registers(3, 3, slave=slave_id)
        
        if result.isError():
            print("❌ Error al leer registros del dispositivo")
            print("   Verificar:")
            print("   - Conexiones físicas (RX, TX, DE/RE)")
            print("   - Slave ID correcto")
            print("   - Transceptor RS-485 funcionando")
            return False
        
        device_id = result.registers[0]
        firmware_ver = result.registers[1] / 100.0
        status = result.registers[2]
        
        print(f"✓ Device ID:      {device_id}")
        print(f"✓ Firmware:       v{firmware_ver:.2f}")
        print(f"✓ Status:         {'OK' if status == 1 else 'Error'}")
        
        return True
    
    except Exception as e:
        print(f"❌ Excepción al leer: {e}")
        return False

def read_sensors(client, slave_id):
    """Leer valores de sensores"""
    print("\nLeyendo sensores...")
    print("-" * 60)
    
    try:
        # Leer registros 0, 1, 2 (Temperatura, Humedad, Presión)
        result = client.read_holding_registers(0, 3, slave=slave_id)
        
        if result.isError():
            print("❌ Error al leer sensores")
            return False
        
        temperatura = result.registers[0] / 100.0
        humedad = result.registers[1] / 100.0
        presion = result.registers[2]
        
        print(f"✓ Temperatura:    {temperatura:.2f} °C")
        print(f"✓ Humedad:        {humedad:.2f} %")
        print(f"✓ Presión:        {presion} hPa")
        
        return True
    
    except Exception as e:
        print(f"❌ Excepción al leer sensores: {e}")
        return False

def test_led_control(client, slave_id):
    """Probar control del LED"""
    print("\nProbando control del LED...")
    print("-" * 60)
    
    try:
        # Encender LED
        print("  Encendiendo LED...")
        result = client.write_register(6, 1, slave=slave_id)
        if result.isError():
            print("❌ Error al encender LED")
            return False
        time.sleep(1)
        
        # Leer estado
        result = client.read_holding_registers(6, 1, slave=slave_id)
        if result.registers[0] == 1:
            print("✓ LED encendido correctamente")
        
        time.sleep(1)
        
        # Apagar LED
        print("  Apagando LED...")
        result = client.write_register(6, 0, slave=slave_id)
        if result.isError():
            print("❌ Error al apagar LED")
            return False
        time.sleep(1)
        
        # Leer estado
        result = client.read_holding_registers(6, 1, slave=slave_id)
        if result.registers[0] == 0:
            print("✓ LED apagado correctamente")
        
        return True
    
    except Exception as e:
        print(f"❌ Excepción en control LED: {e}")
        return False

def test_sample_rate(client, slave_id):
    """Probar configuración de tasa de muestreo"""
    print("\nProbando configuración de tasa de muestreo...")
    print("-" * 60)
    
    try:
        # Leer tasa actual
        result = client.read_holding_registers(7, 1, slave=slave_id)
        if result.isError():
            print("❌ Error al leer tasa de muestreo")
            return False
        
        current_rate = result.registers[0]
        print(f"  Tasa actual: {current_rate} ms")
        
        # Configurar nueva tasa (2000 ms)
        new_rate = 2000
        print(f"  Configurando nueva tasa: {new_rate} ms...")
        result = client.write_register(7, new_rate, slave=slave_id)
        if result.isError():
            print("❌ Error al escribir tasa de muestreo")
            return False
        
        # Verificar
        result = client.read_holding_registers(7, 1, slave=slave_id)
        if result.registers[0] == new_rate:
            print(f"✓ Tasa configurada correctamente: {new_rate} ms")
        
        # Restaurar tasa original
        print(f"  Restaurando tasa original: {current_rate} ms...")
        client.write_register(7, current_rate, slave=slave_id)
        
        return True
    
    except Exception as e:
        print(f"❌ Excepción en configuración: {e}")
        return False

def main():
    # Configuración por defecto
    port = '/dev/ttyUSB0'
    baudrate = 9600
    slave_id = 1
    
    # Parsear argumentos
    if len(sys.argv) > 1:
        port = sys.argv[1]
    if len(sys.argv) > 2:
        baudrate = int(sys.argv[2])
    if len(sys.argv) > 3:
        slave_id = int(sys.argv[3])
    
    print_header()
    
    # Conectar
    client = test_connection(port, baudrate, slave_id)
    if not client:
        sys.exit(1)
    
    # Ejecutar pruebas
    tests_passed = 0
    tests_total = 5
    
    if read_device_info(client, slave_id):
        tests_passed += 1
    
    if read_sensors(client, slave_id):
        tests_passed += 1
    
    if test_led_control(client, slave_id):
        tests_passed += 1
    
    if test_sample_rate(client, slave_id):
        tests_passed += 1
    
    # Lectura continua de muestra
    print("\nLectura continua (5 muestras)...")
    print("-" * 60)
    for i in range(5):
        result = client.read_holding_registers(0, 3, slave=slave_id)
        if not result.isError():
            temp = result.registers[0] / 100.0
            hum = result.registers[1] / 100.0
            pres = result.registers[2]
            print(f"  Muestra {i+1}: T={temp:.2f}°C, H={hum:.2f}%, P={pres}hPa")
            tests_passed += 0.2
        time.sleep(1)
    
    # Cerrar conexión
    client.close()
    
    # Resumen
    print("\n" + "=" * 60)
    print("RESUMEN DE PRUEBAS")
    print("=" * 60)
    print(f"Pruebas exitosas: {int(tests_passed)}/{tests_total}")
    
    if tests_passed >= tests_total:
        print("\n✓ TODAS LAS PRUEBAS PASARON EXITOSAMENTE")
        print("\nEl sensor ESP32-C3 Modbus RTU está funcionando correctamente.")
        sys.exit(0)
    else:
        print("\n⚠ ALGUNAS PRUEBAS FALLARON")
        print("\nConsultar TROUBLESHOOTING.md para más información.")
        sys.exit(1)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\nInterrumpido por usuario")
        sys.exit(1)
