# tren_buzzer

ESP32 que reproduce un sonido de tren en una bocina Bluetooth (A2DP), disparado por un botón.

## Hardware

- ESP32 (`board = esp32dev`)
- Bocina Bluetooth externa
- Botón momentáneo: GPIO4 ↔ GND

## Uso

- Presión corta: suena el tren.
- Mantener presionado 8+ seg: busca y guarda una bocina nueva (MAC en NVS), reconecta sola después.
- Beep de confirmación cada vez que conecta.

## Compilar y subir

```bash
cd tren_buzzer
pio run --target upload --upload-port /dev/ttyUSB0
```

## Notas

- `platform = espressif32@6.5.0` fijo (core nuevo crashea BT clásico con WiFi).
- Alimentar con cargador de pared, no puerto USB de PC (Bluetooth consume más corriente de la que da el puerto, sobre todo el modo de búsqueda).
- `/dev/ttyUSB0` sin permisos: `sudo usermod -aG dialout $USER` (requiere reiniciar sesión).
