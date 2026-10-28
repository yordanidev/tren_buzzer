# tren_buzzer

Proyecto PlatformIO para ESP32 que controla un buzzer activo/pasivo por PWM (LEDC). Pensado como base para una alarma de paso de tren: hoy el firmware es un test de sonido (sirena tipo alarma aérea); la lógica real de detección (sensor IR) todavía no está integrada.

## Hardware

- Placa: ESP32 (genérico, `board = esp32dev`)
- Buzzer: conectado a **GPIO 16**, manejado por PWM vía `ledcAttach`/`ledcWriteTone`
- Sensor IR reflectivo (pendiente de integrar): módulo de 3 pines reutilizado de un ventilador (detecta el paso de un objeto frente a él, p. ej. aspas)
  - VCC → 3V3
  - GND → GND
  - OUT → GPIO4 (propuesto, libre)

## Estado actual del firmware (`src/main.cpp`)

El `loop()` genera una sirena continua: barre la frecuencia del buzzer entre 400Hz y 1200Hz subiendo y bajando (`sweep()`), simulando una alarma tipo sirena de ataque aéreo. No depende de ningún sensor todavía.

Parámetros ajustables al inicio del archivo:
- `FREQ_MIN` / `FREQ_MAX`: rango de frecuencias del barrido
- `STEP`: tamaño de paso en Hz por iteración
- `STEP_DELAY_MS`: velocidad del barrido (menor = sirena más rápida)

## Pendiente (lógica real del tren)

Falta definir y conectar:
1. Lectura del sensor IR en GPIO4 (`digitalRead`)
2. Regla de disparo: qué hace el buzzer cuando el sensor detecta algo (alarma continua mientras detecta, un bip por pulso, o alarma con retardo tras la última detección)

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
