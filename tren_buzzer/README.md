# tren_buzzer

Proyecto PlatformIO para ESP32 que reproduce el sonido de un tren en loop continuo en una bocina Bluetooth externa, vía A2DP (el ESP32 actúa como fuente de audio). Sin WiFi, sin servidor, sin botón — solo Bluetooth.

Versiones anteriores (buzzer con sirena, receptor de control remoto IR, página web con botón, streaming en vivo desde la PC) quedaron en el historial de git; se simplificó todo a lo esencial.

## Hardware

- Placa: ESP32 (genérico, `board = esp32dev`, chip ESP32-D0WD-V3 — soporta Bluetooth clásico)
- Bocina Bluetooth externa (nombre configurado en `SPEAKER_BT_NAME` en `src/main.cpp`)

## Firmware (`src/main.cpp`)

Al arrancar, se conecta por Bluetooth clásico (A2DP source) a la bocina por nombre, y reproduce el audio embebido (`train_pcm` en `src/audio_data.cpp`, generado a partir de `sounds/tren-7.mp3`) en loop continuo sin parar.

**Regenerar el audio embebido** (si se cambia el MP3 de origen en `sounds/`): convertir a PCM 44.1kHz/16-bit estéreo con `gst-launch-1.0 filesrc location=<mp3> ! decodebin ! audioconvert ! audioresample ! audio/x-raw,format=S16LE,channels=2,rate=44100 ! wavenc ! filesink location=<wav>`, y luego volcar los bytes crudos del WAV (sin el header de 44 bytes) como arreglo `const uint8_t train_pcm[]` en `src/audio_data.cpp` / `include/audio_data.h`.

## Notas importantes de hardware/firmware

- **Plataforma fijada a `espressif32@6.5.0`** (Arduino core 2.0.x / IDF 4.4) en `platformio.ini`. Con la plataforma más reciente (core 3.x / IDF 5.x) el Bluetooth clásico crasheaba con `assert failed: host_recv_pkt_cb hci_hal_h4.c:662` al usarse junto con WiFi — un problema de compatibilidad documentado entre el core nuevo y esta librería de BT clásico. Al quitar WiFi de este firmware, es posible que ya no sea necesario, pero se deja fijado por seguridad.
- **Particiones**: el audio embebido + el stack de Bluetooth superan los 1.3MB de la partición de app por defecto, por eso `board_build.partitions = huge_app.csv`.
- **Alimentación**: activar Bluetooth clásico demanda un pico de corriente que un puerto/hub USB débil de PC puede no entregar, causando un bucle de reinicio (brownout, no daña la placa). Para uso real, alimentar con un cargador de pared USB (5V/1A+) en vez del puerto de la PC.

## Compilar y subir

```bash
cd tren_buzzer
pio run --target upload --upload-port /dev/ttyUSB0
```

Monitor serial (interactivo, no funciona con salida redirigida/pipe):

```bash
pio device monitor --port /dev/ttyUSB0 --baud 115200
```

## Problemas conocidos

- **Permission denied en `/dev/ttyUSB0`**: el usuario no está en el grupo `dialout`, así que tras desconectar/reconectar el ESP32 el nuevo nodo de dispositivo puede quedar sin acceso. Solución permanente:
  ```bash
  sudo usermod -aG dialout $USER
  ```
  (requiere cerrar sesión y volver a entrar para que aplique).
