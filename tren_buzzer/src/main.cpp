#include <Arduino.h>
#include <Preferences.h>
#include "BluetoothA2DPSource.h"
#include "audio_data.h"

#define BUTTON_PIN 4
#define DEBOUNCE_MS 50
#define LONG_PRESS_MS 3000
#define PAIRING_TIMEOUT_MS 30000

Preferences prefs;
BluetoothA2DPSource a2dp_source;
uint32_t pos = 0;
bool isPlaying = false;

int buttonState = HIGH;
int lastReading = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long pressStartTime = 0;
bool longPressTriggered = false;

bool pairingMode = false;
unsigned long pairingStartTime = 0;
esp_bd_addr_t foundAddress;
volatile bool addressCaptured = false;

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

bool accept_first_device(const char *ssid, esp_bd_addr_t address, int rrsi) {
  Serial.print("Bocina encontrada: ");
  Serial.println(ssid);
  memcpy(foundAddress, address, sizeof(esp_bd_addr_t));
  addressCaptured = true;
  return true;
}

void startPairingMode() {
  Serial.println("Modo de busqueda: conectando a la primera bocina disponible...");
  pairingMode = true;
  addressCaptured = false;
  pairingStartTime = millis();
  a2dp_source.set_ssid_callback(accept_first_device);
  a2dp_source.start();
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  prefs.begin("train", false);

  a2dp_source.set_data_callback(get_sound_data);

  size_t savedLen = prefs.getBytesLength("speaker_addr");
  if (savedLen == sizeof(esp_bd_addr_t)) {
    esp_bd_addr_t savedAddr;
    prefs.getBytes("speaker_addr", savedAddr, sizeof(esp_bd_addr_t));
    Serial.println("Reconectando a bocina guardada...");
    a2dp_source.set_auto_reconnect(savedAddr);
    a2dp_source.start();
  } else {
    Serial.println("No hay bocina guardada.");
    startPairingMode();
  }
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
        pressStartTime = millis();
        longPressTriggered = false;
        pos = 0;
        isPlaying = true;
        Serial.println("Boton presionado: reproduciendo tren");
      }
    }
  }
  lastReading = reading;

  if (buttonState == LOW && !longPressTriggered &&
      millis() - pressStartTime > LONG_PRESS_MS) {
    longPressTriggered = true;
    isPlaying = false;
    startPairingMode();
  }

  if (pairingMode) {
    if (addressCaptured) {
      prefs.putBytes("speaker_addr", foundAddress, sizeof(esp_bd_addr_t));
      pairingMode = false;
      Serial.println("Bocina guardada.");
    } else if (millis() - pairingStartTime > PAIRING_TIMEOUT_MS) {
      Serial.println("Tiempo de busqueda agotado, sin conectar.");
      pairingMode = false;
    }
  }
}
