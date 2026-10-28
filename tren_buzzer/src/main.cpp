#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "BluetoothA2DPSource.h"
#include "secrets.h"

#define RING_SIZE 65536

static uint8_t ringBuf[RING_SIZE];
static volatile uint32_t ringHead = 0;
static volatile uint32_t ringTail = 0;
static volatile bool streamActive = false;
static volatile bool isPlaying = false;

static char pendingFile[64] = "";
static volatile bool hasPendingFile = false;

BluetoothA2DPSource a2dp_source;

const unsigned long POLL_INTERVAL_MS = 500;

uint32_t ringAvailable() {
  return (ringHead - ringTail + RING_SIZE) % RING_SIZE;
}

uint32_t ringFree() {
  return RING_SIZE - 1 - ringAvailable();
}

void ringPushBytes(const uint8_t *src, size_t len) {
  for (size_t i = 0; i < len; i++) {
    ringBuf[ringHead] = src[i];
    ringHead = (ringHead + 1) % RING_SIZE;
  }
}

int32_t get_sound_data(uint8_t *data, int32_t byteCount) {
  for (int32_t i = 0; i < byteCount; i++) {
    if (ringAvailable() > 0) {
      data[i] = ringBuf[ringTail];
      ringTail = (ringTail + 1) % RING_SIZE;
    } else {
      data[i] = 0;
      if (!streamActive) {
        isPlaying = false;
      }
    }
  }
  return byteCount;
}

void streamTask(void *param) {
  for (;;) {
    if (hasPendingFile) {
      char file[64];
      strncpy(file, pendingFile, sizeof(file));
      hasPendingFile = false;

      ringHead = 0;
      ringTail = 0;
      streamActive = true;
      isPlaying = true;

      Serial.print("Streaming: ");
      Serial.println(file);

      HTTPClient http;
      String url = String(STREAM_URL_BASE) + "?file=" + String(file);
      http.begin(url);
      int code = http.GET();
      if (code == 200) {
        WiFiClient *stream = http.getStreamPtr();
        uint8_t chunk[512];
        while (http.connected()) {
          size_t avail = stream->available();
          if (avail > 0) {
            size_t n = stream->readBytes(chunk, min(avail, sizeof(chunk)));
            size_t written = 0;
            while (written < n) {
              size_t canWrite = min((size_t)ringFree(), n - written);
              if (canWrite == 0) {
                vTaskDelay(1);
                continue;
              }
              ringPushBytes(chunk + written, canWrite);
              written += canWrite;
            }
          } else {
            vTaskDelay(2);
          }
        }
      } else {
        Serial.print("Error HTTP stream: ");
        Serial.println(code);
      }
      http.end();
      streamActive = false;
      Serial.println("Stream terminado");
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
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

  xTaskCreatePinnedToCore(streamTask, "streamTask", 8192, NULL, 1, NULL, 0);
}

void loop() {
  static unsigned long lastPoll = 0;
  unsigned long now = millis();
  if (now - lastPoll >= POLL_INTERVAL_MS) {
    lastPoll = now;
    HTTPClient http;
    http.begin(SHOULD_PLAY_URL);
    int code = http.GET();
    if (code == 200) {
      String body = http.getString();
      if (body != "0" && !hasPendingFile) {
        strncpy(pendingFile, body.c_str(), sizeof(pendingFile) - 1);
        hasPendingFile = true;
      }
    }
    http.end();
  }
}
