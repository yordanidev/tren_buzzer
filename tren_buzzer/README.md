# tren_buzzer

Proyecto PlatformIO para ESP32 que reproduce un sonido de tren en una bocina Bluetooth, controlado desde una página web que el propio ESP32 sirve por WiFi. El buzzer en GPIO16 (sirena por PWM) quedó como firmware anterior en el historial de git; ya no es lo que corre actualmente.

## Hardware

- Placa: ESP32 (genérico, `board = esp32dev`, chip ESP32-D0WD-V3 — soporta Bluetooth clásico)
- Bocina Bluetooth externa (emparejada por nombre, ver `AP_SSID`/nombre de la bocina en `main.cpp`)
- Sin sensores conectados actualmente

## Estado actual del firmware (`src/main.cpp`)

1. El ESP32 crea su propio punto de acceso WiFi (`AP_SSID`/`AP_PASSWORD` al inicio del archivo) y sirve una página con un botón ("SONAR TREN") en `/`.
2. Al presionar el botón desde el celular (conectado a esa red WiFi), se llama a `/play`, que activa la bandera `isPlaying`.
3. En paralelo, el ESP32 se conecta por Bluetooth clásico (A2DP source, librería `ESP32-A2DP` + `arduino-audio-tools`) a la bocina por nombre (`"19-3010"` actualmente, ajustar al nombre real de la bocina).
4. El callback `get_sound_data` transmite el audio embebido (`train_pcm` en `src/audio_data.cpp`, generado a partir de un MP3 convertido a PCM 44.1kHz/16-bit estéreo) una sola vez por cada pulsación del botón; el resto del tiempo manda silencio para mantener el enlace A2DP activo sin repetir el sonido en loop.

**Regenerar el audio embebido** (si se cambia el MP3 de origen): convertir a WAV PCM 44.1kHz/16-bit estéreo (p. ej. con `gst-launch-1.0 filesrc ! decodebin ! audioconvert ! audioresample ! audio/x-raw,format=S16LE,channels=2,rate=44100 ! wavenc ! filesink`), y luego generar `src/audio_data.cpp`/`include/audio_data.h` con un script que vuelque los bytes crudos del WAV (sin el header de 44 bytes) como arreglo `const uint8_t`.

**Importante sobre particiones:** el audio embebido + el stack Bluetooth superan los 1.3MB de la partición de app por defecto. Por eso `platformio.ini` usa `board_build.partitions = huge_app.csv` (da ~3MB a la app, sin slot OTA).

**Importante sobre alimentación:** activar el Bluetooth clásico demanda un pico de corriente que puede causar brownout (bucle de reinicio) si el ESP32 se alimenta de un puerto/hub USB débil. Conectar directo a un puerto USB del equipo, no a un hub.

## Receptor IR (en pausa, no integrado en el firmware activo)

Se intentó integrar un receptor IR (componente marcado `XB336T`, familia TSOP/VS1838B — demodulador de 38kHz para controles remotos, reutilizado de un ventilador), pensado como entrada para activar/desactivar la alarma con cualquier control remoto (código de referencia con la librería `IRremote` quedó en el historial de git, commit `7aa53c5`).

**Hallazgos del diagnóstico de hardware:**
- El componente NO es un sensor de proximidad/reflexión — es un receptor que decodifica señales IR moduladas a 38kHz, como las que emite un control remoto doméstico. No detecta el paso de un objeto físico (tren, aspa) a menos que ese objeto emita/refleje IR modulado a 38kHz.
- Al conectar VCC/GND inicialmente, el ESP32 entraba en un bucle de reinicio (brownout) — se aisló probando primero sin el sensor (estable), luego conectando solo VCC+GND (seguía en bucle), y variando polaridad (seguía en bucle). El bucle de reset en sí no daña la placa (el detector de bajo voltaje solo resetea de forma segura).
- Finalmente, conectando VCC/GND en las patas 2 y 3 (según diagrama del componente) la placa quedó estable. Con OUT (pata 1) conectado a GPIO4, la lectura cruda (`digitalRead`) se mantiene estable en `0`, pero no se detectó ninguna transición a `1` al presionar botones de un control de A/C a distancia normal.
- Pendiente probar: acercar el control a 2-5cm del domo del receptor, o medir con multímetro en modo voltaje DC directo sobre la pata OUT mientras se presiona el control, para descartar que el fotodiodo esté dañado (posible por calor al desoldarlo del ventilador).

**Para reanudar este trabajo:** recuperar el código de `IRremote` del commit `7aa53c5`, confirmar primero con multímetro que el pin OUT reacciona ante un control remoto, y solo entonces volver a conectarlo al ESP32.

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
- Falta instalar `99-platformio-udev.rules` (PlatformIO lo recomienda) para evitar este tipo de problemas de permisos de forma más robusta.
