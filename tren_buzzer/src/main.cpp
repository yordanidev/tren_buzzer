#include <Arduino.h>

#define BUZZER 16

const int FREQ_MIN = 400;
const int FREQ_MAX = 1200;
const int STEP = 10;
const int STEP_DELAY_MS = 8;

void setup() {
  ledcAttach(BUZZER, 2000, 8);
  Serial.begin(115200);
  Serial.println("Test alarma aerea (sirena) en GPIO 16");
}

void sweep(int from, int to) {
  int step = (to > from) ? STEP : -STEP;
  for (int f = from; f != to; f += step) {
    ledcWriteTone(BUZZER, f);
    delay(STEP_DELAY_MS);
  }
  ledcWriteTone(BUZZER, to);
}

void loop() {
  sweep(FREQ_MIN, FREQ_MAX);
  sweep(FREQ_MAX, FREQ_MIN);
}
