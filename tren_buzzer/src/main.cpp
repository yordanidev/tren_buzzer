#include <Arduino.h>
#include "BluetoothA2DPSource.h"
#include "audio_data.h"

#define SPEAKER_BT_NAME "Tronsmart Trip"

BluetoothA2DPSource a2dp_source;
uint32_t pos = 0;

int32_t get_sound_data(uint8_t *data, int32_t byteCount) {
  for (int32_t i = 0; i < byteCount; i++) {
    data[i] = train_pcm[pos];
    pos = (pos + 1) % train_pcm_len;
  }
  return byteCount;
}

void setup() {
  Serial.begin(115200);
  Serial.print("Conectando a bocina Bluetooth ");
  Serial.println(SPEAKER_BT_NAME);
  a2dp_source.set_data_callback(get_sound_data);
  a2dp_source.start(SPEAKER_BT_NAME);
}

void loop() {
}
