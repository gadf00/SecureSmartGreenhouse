#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

// === WiFi ===
const char* ssid = "";
const char* password = "";

// === MQTT ===
const char* mqtt_server = "";
const int mqtt_port = 8883;
const char* mqtt_topic = "esp32/data";

// === CERTIFICATI ===
const char ca_cert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
-----END CERTIFICATE-----
)EOF";

const char client_cert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
-----END CERTIFICATE-----
)EOF";

const char client_key[] PROGMEM = R"EOF(
-----BEGIN PRIVATE KEY-----
-----END PRIVATE KEY-----
)EOF";

// === PIN sensori ===
#define SOIL1_PIN 36
#define SOIL2_PIN 34
#define DHTPIN 4
#define DHTTYPE DHT22

// === Oggetti ===
WiFiClientSecure espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);

// === Timer lettura ===
unsigned long lastRead = 0;
const unsigned long interval = 10000;

void setup() {
  Serial.begin(115200);
  dht.begin();

  WiFi.begin(ssid, password);
  Serial.print("Connection to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  espClient.setCACert(ca_cert);
  espClient.setCertificate(client_cert);
  espClient.setPrivateKey(client_key);

  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastRead >= interval) {
    lastRead = now;
    publishSensorData();
  }
}

void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Connection to MQTT TLS...");
    if (client.connect("IoT_Security_ESP1")) {
      Serial.println("MQTT connected");
    } else {
      Serial.print("MQTT failed: ");
      Serial.println(client.state());
      delay(3000);
    }
  }
}

void publishSensorData() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int rawSoil1 = analogRead(SOIL1_PIN);
  int rawSoil2 = analogRead(SOIL2_PIN);

  if (isnan(temp) || isnan(hum)) {
    Serial.println("DHT reading failed");
    return;
  }

  // Conversione in percentuale
  const int soilWet = 1300;      // valore letto in acqua
  const int soilDry = 4095;      // valore letto in aria

  rawSoil1 = constrain(rawSoil1, soilWet, soilDry);
  rawSoil2 = constrain(rawSoil2, soilWet, soilDry);

  int soilPercent1 = map(rawSoil1, soilDry, soilWet, 0, 100);
  int soilPercent2 = map(rawSoil2, soilDry, soilWet, 0, 100);

  StaticJsonDocument<256> doc;
  doc["plant1"]["soil"] = soilPercent1;
  doc["plant2"]["soil"] = soilPercent2;
  doc["temperature"] = temp;
  doc["humidity"] = hum;

  char payload[256];
  serializeJson(doc, payload);
  Serial.print("Sending: ");
  Serial.println(payload);
  client.publish(mqtt_topic, payload);
}
