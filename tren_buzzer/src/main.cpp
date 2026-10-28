#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "BluetoothA2DPSource.h"
#include "audio_data.h"

const char *AP_SSID = "TrenESP32";
const char *AP_PASSWORD = "tren12345";

BluetoothA2DPSource a2dp_source;
WebServer server(80);

uint32_t pos = 0;
bool isPlaying = false;

const char INDEX_HTML[] = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Tren ESP32</title>
<style>
body { font-family: sans-serif; text-align: center; background: #111; color: #eee; padding-top: 60px; }
button { font-size: 28px; padding: 30px 60px; border-radius: 16px; border: none; background: #e63946; color: white; }
button:active { background: #b3242f; }
</style>
</head>
<body>
<h1>Alarma de tren</h1>
<button onclick="playSound()">SONAR TREN</button>
<p id="status"></p>
<script>
function playSound() {
  document.getElementById('status').innerText = 'Reproduciendo...';
  fetch('/play').then(() => {
    document.getElementById('status').innerText = 'Listo';
  });
}
</script>
</body>
</html>
)HTML";

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

void handlePlay() {
  pos = 0;
  isPlaying = true;
  server.send(200, "text/plain", "ok");
}

void setup() {
  Serial.begin(115200);

  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.print("WiFi AP listo. IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", []() { server.send(200, "text/html", INDEX_HTML); });
  server.on("/play", handlePlay);
  server.begin();

  Serial.println("Conectando a bocina Bluetooth 19-3010...");
  a2dp_source.set_data_callback(get_sound_data);
  a2dp_source.start("19-3010");
}

void loop() {
  server.handleClient();
}
