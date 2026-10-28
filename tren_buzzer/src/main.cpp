#include <Arduino.h>

#define BUZZER 16

const int FREQ_MIN = 400;
const int FREQ_MAX = 1200;
const int STEP = 10;
const unsigned long STEP_DELAY_MS = 8;

int freq = FREQ_MIN;
int direction = 1;
unsigned long lastStepTime = 0;

void setup() {
  ledcAttach(BUZZER, 2000, 8);
  Serial.begin(115200);
  Serial.println("Sirena alarma aerea en GPIO 16");
}

void loop() {
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
