#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// Kredensial Jaringan Wi-Fi
const char* ssid = "Galaxy S24 FE C2D6";
const char* password = "sherlyza";

// Konfigurasi Broker MQTT Publik
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
const char* clientId = "ESP8266_LabIoT_25782065_Tugas";

// Definisi Topik Sesuai NIM 25782065
const char* topicPublishTelemetry = "kampus/labiot/25782065/telemetri";
const char* topicSubscribeKendali = "kampus/labiot/25782065/kendali";
const char* topicPublishAlert     = "kampus/labiot/25782065/alert";
const char* topicPublishStatus    = "kampus/labiot/25782065/status";

// Alokasi Pin Perangkat Keras
const byte dhtPin = 2;   // Pin D4 (GPIO 2)
const byte ledPin = 5;   // Pin D1 (GPIO 5)

#define DHTTYPE DHT22
DHT dht(dhtPin, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastTelemetryTime = 0;
const unsigned long telemetryInterval = 4000;

bool alertSentState = false;

void callback(char* topic, byte* payload, unsigned int length) {
  char incomingBuffer[length + 1];
  memcpy(incomingBuffer, payload, length);
  incomingBuffer[length] = '\0';

  Serial.println("----------------------------------------");
  Serial.printf("[KONTROL DITERIMA] Topik: %s | Payload: %s\n", topic, incomingBuffer);

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, incomingBuffer);
  if (err) {
    Serial.println("Gagal membedah payload JSON!");
    return;
  }

  if (doc.containsKey("led")) {
    float currentTemp = dht.readTemperature();
    if (!isnan(currentTemp) && currentTemp > 31.0) {
      Serial.println("--> [ABAIKAN] Kendali manual dikunci karena status BAHAYA (Overheat)!");
    } else {
      int val = doc["led"];
      digitalWrite(ledPin, val == 1 ? HIGH : LOW);
      Serial.printf("--> [MANUAL OVERRIDE] Status LED diubah: %s\n", val == 1 ? "MENYALA" : "PADAM");
    }
  }
  Serial.println("----------------------------------------");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke Broker MQTT...");
    const char* lwtTopic = topicPublishStatus;
    const char* lwtMessage = "{\"status\":\"OFFLINE\",\"keterangan\":\"Perangkat Terputus Tak Terduga\"}";
    
    if (client.connect(clientId, NULL, NULL, lwtTopic, 1, true, lwtMessage)) {
      Serial.println(" Sukses Terhubung!");
      String onlineMsg = "{\"status\":\"ONLINE\",\"keterangan\":\"Perangkat Beroperasi Normal\"}";
      client.publish(topicPublishStatus, onlineMsg.c_str(), true);
      client.subscribe(topicSubscribeKendali);
    } else {
      Serial.printf(" Gagal (rc=%d). Coba lagi dalam 4 detik...\n", client.state());
      delay(4000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  dht.begin();
  delay(2000);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Terkoneksi!");

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastTelemetryTime >= telemetryInterval) {
    lastTelemetryTime = now;
    
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    // Hanya cek apakah nilainya benar-benar NaN (kabel terlepas total)
    if (isnan(t) || isnan(h)) {
      Serial.println("[WARNING] Gagal membaca data dari sensor DHT!");
      return;
    }

    String kondisiSuhu = (t <= 31.0) ? "NORMAL" : "PANAS";

    StaticJsonDocument<256> docOut;
    docOut["node_id"]      = clientId;
    docOut["suhu"]         = serialized(String(t, 2));
    docOut["kelembapan"]   = serialized(String(h, 2));
    docOut["kondisi_suhu"] = kondisiSuhu;
    docOut["status_led"]   = digitalRead(ledPin);

    char buffer[256];
    serializeJson(docOut, buffer);
    client.publish(topicPublishTelemetry, buffer);
    Serial.printf("[TELEMETRI TERKIRIM] %s\n", buffer);

    if (t > 31.0) {
      digitalWrite(ledPin, HIGH);
      if (!alertSentState) {
        StaticJsonDocument<256> alertDoc;
        alertDoc["status"]      = "BAHAYA";
        alertDoc["parameter"]   = "OVERHEAT";
        alertDoc["nilai_suhu"]  = serialized(String(t, 2));

        char alertBuffer[256];
        serializeJson(alertDoc, alertBuffer);
        client.publish(topicPublishAlert, alertBuffer, true);
        alertSentState = true;
      }
    } else {
      if (alertSentState) {
        alertSentState = false;
      }
    }
  }
}