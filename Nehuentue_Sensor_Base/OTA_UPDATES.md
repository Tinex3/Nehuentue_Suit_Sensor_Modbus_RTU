# ⚡ OTA Updates - Actualizaciones Over-The-Air

## 📋 Descripción

El firmware incluye **AsyncElegantOTA** para realizar actualizaciones remotas del firmware sin necesidad de conectar el ESP32 por USB.

---

## 🚀 Características

✅ **Interfaz web elegante** en `/update`  
✅ **Sube firmware** (.bin) desde el navegador  
✅ **Barra de progreso** visual en tiempo real  
✅ **Reinicio automático** después de actualización exitosa  
✅ **Seguro**: Validación de firmware antes de flashear  
✅ **Sin cables**: Actualiza desde WiFi  

---

## 🌐 Acceso a OTA

### Desde Dashboard:
1. Abre el navegador en `http://IP_DEL_ESP32/`
2. Haz clic en el botón **"⚡ OTA Update"**

### Directo:
```
http://IP_DEL_ESP32/update
```

**Ejemplo:**
- Modo AP: `http://192.168.4.1/update`
- Modo STA: `http://192.168.1.100/update` (según tu IP local)

---

## 📦 Cómo Compilar el Firmware .bin

### Opción 1: PlatformIO (VS Code)
```bash
# Compilar sin subir
pio run

# El archivo .bin se genera en:
# .pio/build/adafruit_qtpy_esp32c3/firmware.bin
```

### Opción 2: Tarea de VS Code
1. `Ctrl + Shift + P`
2. Buscar: **"PlatformIO: Build"**
3. Archivo generado: `.pio/build/adafruit_qtpy_esp32c3/firmware.bin`

---

## 📤 Cómo Subir Firmware OTA

### Paso a Paso:

1. **Compila el firmware**:
   ```bash
   pio run
   ```

2. **Localiza el archivo .bin**:
   ```
   .pio/build/adafruit_qtpy_esp32c3/firmware.bin
   ```

3. **Accede a la interfaz OTA**:
   - Abre `http://IP_DEL_ESP32/update`

4. **Sube el firmware**:
   - Haz clic en **"Choose File"**
   - Selecciona `firmware.bin`
   - Haz clic en **"Update"**

5. **Espera la actualización**:
   - Verás una barra de progreso
   - Al llegar a 100%, el ESP32 se reiniciará automáticamente

6. **Verifica la actualización**:
   - El ESP32 arrancará con el nuevo firmware
   - Conéctate nuevamente a la interfaz web

---

## ⚠️ Advertencias Importantes

### ❌ NO hagas esto durante OTA:
- ❌ No desconectes la alimentación
- ❌ No cierres el navegador durante la actualización
- ❌ No subas firmware incorrecto (verifica la placa)

### ✅ Mejores Prácticas:
- ✅ **Prueba el firmware** en una placa de desarrollo primero
- ✅ **Verifica la placa** en `platformio.ini` (`adafruit_qtpy_esp32c3`)
- ✅ **Mantén WiFi estable** durante la actualización
- ✅ **Guarda configuración** antes de actualizar (si es posible)
- ✅ **Anota la IP** del ESP32 antes de actualizar

---

## 🔧 Solución de Problemas

### Problema: "Update Failed"
**Causas:**
- Archivo .bin corrupto o incorrecto
- Conexión WiFi inestable
- Firmware muy grande para la partición

**Solución:**
- Recompila el firmware
- Acércate al router WiFi
- Verifica que el firmware no exceda el tamaño de la partición

### Problema: ESP32 no arranca después de OTA
**Causas:**
- Firmware incompatible
- Corrupción durante la subida

**Solución:**
- Conecta por USB y flashea el firmware con PlatformIO:
  ```bash
  pio run --target upload
  ```

### Problema: No puedo acceder a /update
**Causas:**
- ESP32 no conectado a WiFi
- Firewall bloqueando el puerto 80

**Solución:**
- Verifica conexión WiFi en Dashboard
- Intenta desde modo AP: `http://192.168.4.1/update`

---

## 🔐 Seguridad

### Recomendaciones:

1. **Autenticación HTTP** (TODO - futuro):
   ```cpp
   AsyncElegantOTA.begin(&server, "admin", "password");
   ```

2. **Solo en red local**: No expongas el puerto 80 a internet

3. **Firewall**: Configura router para bloquear acceso externo

4. **HTTPS** (TODO - futuro): Cifrar comunicación con certificados

---

## 📊 Estadísticas

| Característica | Valor |
|---------------|-------|
| **Tamaño librería** | ~15 KB Flash |
| **Tiempo actualización** | ~30-60 segundos (depende del tamaño del firmware) |
| **Tamaño máximo firmware** | ~1.3 MB (depende de partición) |
| **Compatibilidad** | ESP32, ESP32-C3, ESP32-S2, ESP32-S3 |

---

## 🎯 Ejemplo Completo

### 1. Compilar:
```bash
cd Nehuentue_Sensor_Base
pio run
```

### 2. Encontrar archivo:
```bash
ls -lh .pio/build/adafruit_qtpy_esp32c3/firmware.bin
# -rw-r--r-- 1 user user 823K Oct 19 12:34 firmware.bin
```

### 3. Subir OTA:
- Abre: `http://192.168.1.100/update`
- Sube: `firmware.bin`
- Espera: Barra de progreso 0% → 100%
- Resultado: ESP32 reinicia con nuevo firmware

---

## 📝 Notas Técnicas

### Integración en el Código:

**platformio.ini:**
```ini
lib_deps = 
    ayushsharma82/AsyncElegantOTA@^2.2.7
```

**main.cpp:**
```cpp
#include <AsyncElegantOTA.h>

void setup() {
    // ... inicialización WiFi y web server ...
    AsyncElegantOTA.begin(&webServer);
}

void loop() {
    AsyncElegantOTA.loop();  // Necesario para procesamiento
}
```

### Particiones ESP32:

El ESP32-C3 usa particiones típicas:
- **App0**: 1.3 MB (firmware actual)
- **App1**: 1.3 MB (firmware nuevo para OTA)
- **Spiffs/LittleFS**: ~256 KB (opcional)

Durante OTA:
1. Descarga firmware a **App1**
2. Valida integridad
3. Marca **App1** como boot
4. Reinicia

Si falla, automáticamente vuelve a **App0** (rollback).

---

## 🔄 Rollback Automático

AsyncElegantOTA incluye **rollback automático**:

1. Si el nuevo firmware **falla al arrancar** 3 veces
2. ESP32 automáticamente vuelve al firmware **anterior**
3. No pierdes el control del dispositivo

---

## ✅ Ventajas vs USB

| Característica | OTA | USB |
|---------------|-----|-----|
| **Acceso físico** | ❌ No necesario | ✅ Necesario |
| **Velocidad** | ~30-60s | ~10-20s |
| **Comodidad** | ✅ Remoto | ❌ Presencial |
| **Múltiples dispositivos** | ✅ Fácil | ❌ Tedioso |
| **Seguridad** | ⚠️ Requiere WiFi | ✅ Más seguro |

---

## 🎓 Referencias

- [AsyncElegantOTA GitHub](https://github.com/ayushsharma82/AsyncElegantOTA)
- [ESP32 OTA Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/ota.html)
- [PlatformIO OTA Guide](https://docs.platformio.org/en/latest/platforms/espressif32.html#over-the-air-ota-update)

---

## 🚀 Próximos Pasos

- [ ] Agregar autenticación HTTP Basic
- [ ] Implementar HTTPS con certificados
- [ ] Backup automático de configuración antes de OTA
- [ ] Notificación MQTT cuando hay nueva versión disponible
- [ ] Servidor de actualización centralizado (auto-update)
