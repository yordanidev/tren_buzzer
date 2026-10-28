# tren_buzzer

Proyecto PlatformIO para ESP32 que controla un buzzer activo/pasivo por PWM (LEDC). Pensado como base para una alarma de paso de tren. Por ahora el firmware activo es solo el test de sonido (sirena tipo alarma aérea); la integración del sensor IR quedó pausada por un problema de hardware sin resolver (ver abajo).

## Hardware

- Placa: ESP32 (genérico, `board = esp32dev`)
- Buzzer: conectado a **GPIO 16**, manejado por PWM vía `ledcAttach`/`ledcWriteTone`

## Estado actual del firmware (`src/main.cpp`)

El `loop()` genera una sirena continua: barre la frecuencia del buzzer entre 400Hz y 1200Hz subiendo y bajando, sin depender de ningún sensor.

Parámetros ajustables al inicio del archivo:
- `FREQ_MIN` / `FREQ_MAX`: rango de frecuencias del barrido
- `STEP`: tamaño de paso en Hz por iteración
- `STEP_DELAY_MS`: velocidad del barrido (menor = sirena más rápida)

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
