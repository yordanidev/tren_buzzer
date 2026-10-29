# tren_buzzer

Proyecto PlatformIO para ESP32 que reproduce el sonido de un tren en una bocina Bluetooth externa vía A2DP (el ESP32 actúa como fuente de audio), disparado por un botón físico. Sin WiFi, sin servidor en PC — solo Bluetooth + un botón.

Versiones anteriores (buzzer con sirena, receptor de control remoto IR, página web con botón, streaming en vivo desde la PC) quedaron en el historial de git; se simplificó todo a lo esencial.

## Hardware

- Placa: ESP32 (genérico, `board = esp32dev`, chip ESP32-D0WD-V3 — soporta Bluetooth clásico)
- Bocina Bluetooth externa
- Botón momentáneo (pull, regresa solo): una pata a **GPIO4**, la otra a **GND** (usa el pull-up interno, no necesita resistencia)

## Firmware (`src/main.cpp`)

- **Presión corta del botón**: reproduce el audio embebido (`train_pcm` en `src/audio_data.cpp`, generado a partir de `sounds/tren-7.mp3`) una vez vía A2DP.
- **Mantener presionado 3+ segundos**: entra en modo de búsqueda — se conecta por Bluetooth a la primera bocina disponible que encuentre (sin necesitar saber su nombre), guarda su **dirección MAC** en la memoria permanente del ESP32 (NVS, vía `Preferences`), y la usa automáticamente en los siguientes arranques (`set_auto_reconnect`).
- Al arrancar: si hay una bocina guardada, reconecta directo a esa dirección; si no, entra en modo de búsqueda automáticamente.

**Por qué MAC y no nombre**: se intentó primero guardar el nombre de la bocina descubierta (vía `get_name()`), pero esa llamada devolvía un valor corrupto durante el descubrimiento "a ciegas" (sin nombre conocido de antemano) — causaba que el ESP32 intentara reconectarse sin parar a una bocina con un nombre inexistente. Se cambió a capturar la dirección MAC directamente del callback de descubrimiento (`set_ssid_callback`, que recibe la dirección como parámetro) y usar `connect_to`/`set_auto_reconnect`, que es confiable.

**Regenerar el audio embebido** (si se cambia el MP3 de origen en `sounds/`): convertir a PCM 44.1kHz/16-bit estéreo con `gst-launch-1.0 filesrc location=<mp3> ! decodebin ! audioconvert ! audioresample ! audio/x-raw,format=S16LE,channels=2,rate=44100 ! wavenc ! filesink location=<wav>`, y luego volcar los bytes crudos del WAV (sin el header de 44 bytes) como arreglo `const uint8_t train_pcm[]` en `src/audio_data.cpp` / `include/audio_data.h`.

## Notas importantes de hardware/firmware

- **Plataforma fijada a `espressif32@6.5.0`** (Arduino core 2.0.x / IDF 4.4) en `platformio.ini`. Con la plataforma más reciente (core 3.x / IDF 5.x) el Bluetooth clásico crasheaba con `assert failed: host_recv_pkt_cb hci_hal_h4.c:662` al usarse junto con WiFi — un problema de compatibilidad documentado entre el core nuevo y esta librería de BT clásico. Al quitar WiFi de este firmware, es posible que ya no sea necesario, pero se deja fijado por seguridad.
- **Particiones**: el audio embebido + el stack de Bluetooth superan los 1.3MB de la partición de app por defecto, por eso `board_build.partitions = huge_app.csv`.
- **Alimentación**: activar Bluetooth clásico demanda un pico de corriente que un puerto/hub USB débil de PC puede no entregar, causando un bucle de reinicio (brownout, no daña la placa). El **modo de búsqueda/escaneo** (al mantener el botón presionado) consume todavía más corriente que la reconexión normal — en el puerto USB de la PC causa brownout incluso cuando la reconexión normal funciona bien ahí. Para uso real, y siempre para probar el modo de búsqueda, alimentar con un cargador de pared USB (5V/1A+) en vez del puerto de la PC.

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
