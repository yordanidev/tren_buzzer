#include <Arduino.h>
#include <IRremote.hpp>

#define BUZZER 16
#define IR_RECEIVE_PIN 4

const int FREQ_MIN = 400;
const int FREQ_MAX = 1200;
const int STEP = 10;
const unsigned long STEP_DELAY_MS = 8;

int freq = FREQ_MIN;
int direction = 1;
unsigned long lastStepTime = 0;
bool alarmOn = false;

void setup() {
  ledcAttach(BUZZER, 2000, 8);
  Serial.begin(115200);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  Serial.println("Receptor IR listo en GPIO4. Cualquier boton del control activa/desactiva la alarma.");
}

void loop() {
  if (IrReceiver.decode()) {
    alarmOn = !alarmOn;
    Serial.println(alarmOn ? "Control remoto: ALARMA ON" : "Control remoto: ALARMA OFF");
    if (!alarmOn) {
      ledcWriteTone(BUZZER, 0);
      freq = FREQ_MIN;
      direction = 1;
    }
    IrReceiver.resume();
  }

  if (!alarmOn) return;

  unsigned long now = millis();
  if (now - lastStepTime >= STEP_DELAY_MS) {
    lastStepTime = now;
    ledcWriteTone(BUZZER, freq);
    freq += direction * STEP;
    if (freq >= FREQ_MAX) {
      freq = FREQ_MAX;
      direction = -1;
    } else if (freq <= FREQ_MIN) {
      freq = FREQ_MIN;
      direction = 1;
    }
  }
}
