#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>

const char* ssid = "Galaxy S24 FE C2D6";
const char* password = "sherlyza";

#define DHTPIN 4
#define DHTTYPE DHT22  
DHT dht(DHTPIN, DHTTYPE);

const int pinLED = 5;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Smart Lighting - PWM Slider</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; background: #f4f4f9; }
    .card { background: white; padding: 20px; border-radius: 10px; box-shadow: 0px 0px 10px rgba(0,0,0,0.1); display: inline-block; margin: 10px; width: 300px; }
    input[type=range] { width: 100%; cursor: pointer; }
    .value-text { font-size: 18px; font-weight: bold; color: #007BFF; }
  </style>
</head>
<body>
  <h2>Smart Lighting (Modul 4)</h2>
  
  <div class="card">
    <h3>Dimmer Lampu LED</h3>
    <input type="range" min="0" max="1023" id="pwmSlider" onchange="sendPWM(this.value)">
    <p>Nilai PWM: <span id="sliderVal" class="value-text">0</span></p>
  </div>

  <div class="card">
    <h3>Sensor DHT</h3>
    <p>Suhu: <span id="temp" class="value-text">--</span> &deg;C</p>
    <p>Kelembapan: <span id="hum" class="value-text">--</span> %</p>
  </div>

  <script>
    var gateway = `ws://${window.location.hostname}/ws`;
    var websocket;

    window.addEventListener('load', function() {
      websocket = new WebSocket(gateway);
      websocket.onmessage = onMessage;
    });

    function sendPWM(value) {
      document.getElementById("sliderVal").innerHTML = value;
      websocket.send("pwm," + value);
    }

    function onMessage(event) {
      var data = JSON.parse(event.data);
      if (data.temperature !== undefined) document.getElementById("temp").innerHTML = data.temperature;
      if (data.humidity !== undefined) document.getElementById("hum").innerHTML = data.humidity;
    }
  </script>
</body>
</html>
)rawliteral";

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String message = (char*)data;
    
    if (message.startsWith("pwm,")) {
      int nilaiPWM = message.substring(4).toInt();
      nilaiPWM = constrain(nilaiPWM, 0, 1023);
      
      analogWrite(pinLED, nilaiPWM);
      
      Serial.print("Nilai PWM Diterima: ");
      Serial.println(nilaiPWM);
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_DATA) {
    handleWebSocketMessage(arg, data, len);
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(pinLED, OUTPUT);
  analogWriteRange(1023);
  
  dht.begin();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Terhubung! IP: " + WiFi.localIP().toString());

  ws.onEvent(onEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  server.begin();
}

void loop() {
  ws.cleanupClients();

  static unsigned long lastMillis = 0;
  if (millis() - lastMillis > 2000) {
    lastMillis = millis();
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    
    if (!isnan(h) && !isnan(t)) {
      String json = "{\"temperature\":" + String(t, 1) + ",\"humidity\":" + String(h, 1) + "}";
      ws.textAll(json);
    }
  }
}