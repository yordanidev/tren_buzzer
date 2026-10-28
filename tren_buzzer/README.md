# tren_buzzer

Proyecto PlatformIO para ESP32 que reproduce sonidos (tren, bocina de barco) en una bocina Bluetooth externa, disparados desde una página web que corre en la PC. El ESP32 es cliente WiFi (no access point) y cliente Bluetooth A2DP (fuente de audio). El buzzer en GPIO16 (sirena por PWM) y el receptor de control remoto IR quedaron como firmwares anteriores en el historial de git; ya no es lo que corre actualmente.

**⚠️ Bloqueado por hardware — ver "Problema de alimentación" abajo.** El código compila y sube bien, pero el ESP32 entra en bucle de reinicio (brownout) al activar WiFi+Bluetooth simultáneamente alimentado desde el puerto USB de la PC. No se ha podido validar en hardware real todavía.

## Arquitectura

```
Celular/navegador --HTTP--> pc_server/app.py (PC, puerto 5000)
                                   |
                                   | ESP32 hace polling GET /should_play cada 500ms
                                   v
                              ESP32 (WiFi STA + Bluetooth A2DP source)
                                   |
                                   v
                          Bocina Bluetooth externa ("19-3010")
```

- **`pc_server/app.py`**: servidor HTTP simple (sin dependencias, `http.server` de la librería estándar) que corre en la PC. Sirve una página en `/` con un botón por sonido (`SOUNDS = ["tren", "barco"]`). `/play?sound=<nombre>` marca ese sonido como pendiente; `/should_play` lo devuelve una vez y lo limpia (polling, no websockets).
- **`src/main.cpp`**: el ESP32 se conecta a la red WiFi existente (`WIFI_SSID`/`WIFI_PASSWORD`) y a la bocina Bluetooth por nombre. Hace polling a `SHOULD_PLAY_URL` (IP de la PC) cada `POLL_INTERVAL_MS`; si la respuesta coincide con el nombre de un sonido en el arreglo `sounds[]`, lo reproduce una vez vía el callback `get_sound_data` del A2DP source, y manda silencio el resto del tiempo.
- **`sounds/`**: MP3 originales guardados como referencia (`tren-7.mp3`, `bocina-barco.mp3`).
- **`tools/convert_sound.py`**: convierte un MP3 nuevo a PCM 44.1kHz/16-bit estéreo y lo agrega a `src/audio_data.cpp`/`include/audio_data.h`. Uso: `python3 tools/convert_sound.py <nombre_variable> sounds/<archivo>.mp3` (requiere `gst-launch-1.0` con plugin `mpg123audiodec` o `avdec_mp3` para decodificar MP3). Para agregar un sonido nuevo hay que además: 1) agregarlo a `SOUNDS` en `pc_server/app.py`, 2) agregarlo al arreglo `sounds[]` en `src/main.cpp`.

**Importante sobre particiones:** el audio embebido + el stack Bluetooth superan los 1.3MB de la partición de app por defecto. Por eso `platformio.ini` usa `board_build.partitions = huge_app.csv` (da ~3MB a la app, sin slot OTA). Si se cambia este esquema de particiones en un ESP32 que ya tenía otro, hay que borrar la flash completa (`pio run --target erase`) antes de subir, o puede quedar en un estado inconsistente.

## Problema de alimentación (sin resolver)

Activar WiFi + Bluetooth clásico simultáneamente demanda un pico de corriente que el puerto USB de la PC (incluso directo, sin hub) no puede entregar de forma estable. Síntoma: bucle de reinicio (brownout) continuo, visible como texto repetido tipo `try 0x4008059c` o como ruido binario en el serial (según en qué punto del arranque ocurre el reset). No es daño a la placa — el detector de bajo voltaje solo resetea de forma segura — pero impide que el firmware llegue a correr.

Se descartó que fuera un problema de particiones corruptas (se probó borrando la flash completa y resubiendo; el bucle persistió igual).

**Siguiente paso para desbloquear:** alimentar el ESP32 con un cargador de pared USB (5V/1A o más) en vez del puerto de la PC, o agregar un capacitor electrolítico de 470-1000µF entre 3V3 y GND cerca de la placa para absorber los picos de corriente.

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
