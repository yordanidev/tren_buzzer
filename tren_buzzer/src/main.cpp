#include <Arduino.h>
#include "BluetoothA2DPSource.h"
#include "audio_data.h"

#define SPEAKER_BT_NAME "19-3010"
#define BUTTON_PIN 4
#define DEBOUNCE_MS 50

BluetoothA2DPSource a2dp_source;
uint32_t pos = 0;
bool isPlaying = false;

int buttonState = HIGH;
int lastReading = HIGH;
unsigned long lastDebounceTime = 0;

int32_t get_sound_data(uint8_t *data, int32_t byteCount) {
  if (!isPlaying) {
    memset(data, 0, byteCount);
    return byteCount;
  }
  for (int32_t i = 0; i < byteCount; i++) {
    data[i] = train_pcm[pos];
    pos++;
    if (pos >= train_pcm_len) {
      pos = 0;
      isPlaying = false;
      memset(data + i + 1, 0, byteCount - i - 1);
      break;
    }
  }
  return byteCount;
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.print("Conectando a bocina Bluetooth ");
  Serial.println(SPEAKER_BT_NAME);
  a2dp_source.set_data_callback(get_sound_data);
  a2dp_source.start(SPEAKER_BT_NAME);
}

void loop() {
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastReading) {
    lastDebounceTime = millis();
  }

  if (millis() - lastDebounceTime > DEBOUNCE_MS) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        pos = 0;
        isPlaying = true;
        Serial.println("Boton presionado: reproduciendo tren");
      }
    }
  }

  lastReading = reading;
}
