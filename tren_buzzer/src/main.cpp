#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "BluetoothA2DPSource.h"
#include "audio_data.h"
#include "secrets.h"

const unsigned long POLL_INTERVAL_MS = 500;

struct Sound {
  const char *name;
  const uint8_t *pcm;
  uint32_t len;
};

Sound sounds[] = {
  {"tren", tren_pcm, tren_pcm_len},
  {"barco", barco_pcm, barco_pcm_len},
};
const int SOUND_COUNT = sizeof(sounds) / sizeof(sounds[0]);

BluetoothA2DPSource a2dp_source;
uint32_t pos = 0;
bool isPlaying = false;
int currentSound = 0;
unsigned long lastPoll = 0;

int32_t get_sound_data(uint8_t *data, int32_t byteCount) {
  if (!isPlaying) {
    memset(data, 0, byteCount);
    return byteCount;
  }
  const Sound &s = sounds[currentSound];
  for (int32_t i = 0; i < byteCount; i++) {
    data[i] = s.pcm[pos];
    pos++;
    if (pos >= s.len) {
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

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi conectado, IP: ");
  Serial.println(WiFi.localIP());

  Serial.print("Conectando a bocina Bluetooth ");
  Serial.println(SPEAKER_BT_NAME);
  a2dp_source.set_data_callback(get_sound_data);
  a2dp_source.start(SPEAKER_BT_NAME);
}

void loop() {
  unsigned long now = millis();
  if (now - lastPoll >= POLL_INTERVAL_MS) {
    lastPoll = now;
    HTTPClient http;
    http.begin(SHOULD_PLAY_URL);
    int code = http.GET();
    if (code == 200) {
      String body = http.getString();
      for (int i = 0; i < SOUND_COUNT; i++) {
        if (body == sounds[i].name) {
          pos = 0;
          currentSound = i;
          isPlaying = true;
          Serial.print("Reproduciendo: ");
          Serial.println(sounds[i].name);
          break;
        }
      }
    }
    http.end();
  }
}
