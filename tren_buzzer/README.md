# tren_buzzer

Proyecto PlatformIO para ESP32 que controla un buzzer activo/pasivo por PWM (LEDC), activado por un receptor de control remoto IR. Pensado como base para una alarma de paso de tren: cualquier botón de un control remoto IR activa/desactiva una sirena tipo alarma aérea.

## Hardware

- Placa: ESP32 (genérico, `board = esp32dev`)
- Buzzer: conectado a **GPIO 16**, manejado por PWM vía `ledcAttach`/`ledcWriteTone`
- Receptor IR (componente marcado `XB336T`, familia TSOP/VS1838B — demodulador de 38kHz para controles remotos, reutilizado de un ventilador): 3 pines
  - VCC → 3V3
  - GND → GND
  - OUT → **GPIO 4**

**Nota sobre el sensor:** este componente NO es un sensor de proximidad/reflexión — es un receptor que decodifica señales IR moduladas a 38kHz, como las que emite cualquier control remoto doméstico. No sirve para detectar directamente el paso de un objeto (tren, aspa) a menos que ese objeto emita o refleje una señal IR modulada a 38kHz. Por eso la lógica actual lo usa como lo que realmente es: un receptor de control remoto.

## Estado actual del firmware (`src/main.cpp`)

Usa la librería `IRremote` para decodificar cualquier señal recibida en GPIO4:
- Cada vez que se detecta una pulsación de botón de un control remoto (cualquier botón, de cualquier protocolo), se alterna el estado `alarmOn`.
- Mientras `alarmOn` esté activo, el buzzer suena una sirena continua (barrido de frecuencia entre 400Hz y 1200Hz, subiendo y bajando).
- Al desactivarse, el buzzer se apaga de inmediato.

Parámetros ajustables al inicio del archivo:
- `FREQ_MIN` / `FREQ_MAX`: rango de frecuencias del barrido
- `STEP`: tamaño de paso en Hz por iteración
- `STEP_DELAY_MS`: velocidad del barrido (menor = sirena más rápida)

## Pendiente

- Si se quiere detectar el paso físico real del tren (no solo encender/apagar con un control), hace falta un sensor distinto: un módulo reflectivo simple (p. ej. TCRT5000 o fotointerruptor) en vez de este receptor de control remoto.
- Evaluar si conviene distinguir botones específicos del control (usando `IrReceiver.decodedIRData.command`) en vez de que cualquier botón alterne la alarma.

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
