#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>


#define RELAY1 25
#define RELAY2 26

// === Wi-Fi ===
const char* ssid = "";
const char* password = "";

// === MQTT ===
const char* mqtt_server = "";
const int mqtt_port = 8883;
const char* topic_command = "esp32/command";
const char* topic_confirm = "esp32/confirm";

// === TLS Certificates ===
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

// === Variabili ===
WiFiClientSecure espClient;
PubSubClient client(espClient);

// Timestamp ultime irrigazioni
time_t lastIrrigation1 = 0;
time_t lastIrrigation2 = 0;

void setup() {
  Serial.begin(115200);

  // Relay GPIO setup
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  digitalWrite(RELAY1, HIGH); 
  digitalWrite(RELAY2, HIGH);

  // Connessione Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connection to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // Impostazioni fuso orario (Italia: UTC+2 con ora legale)
  configTime(3600 * 2, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Time synchronizing...");
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nTime synchronized!");

  // TLS
  espClient.setCACert(ca_cert);
  espClient.setCertificate(client_cert);
  espClient.setPrivateKey(client_key);

  // MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

}

void loop() {
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();
}

void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Connection to MQTT TLS...");
    if (client.connect("IoT_Security_ESP2")) {
      Serial.println("Connected to MQTT");
      client.subscribe(topic_command);
    } else {
      Serial.print("MQTT Error: ");
      Serial.println(client.state());
      delay(3000);
    }
  }
}

void sendConfirmation(String pump_id, int command_id, String type = "manual") {
  StaticJsonDocument<256> doc;

  doc["pump_id"] = pump_id;
  doc["status"] = "done";
  doc["command_id"] = command_id;
  doc["timestamp"] = formatDateTime(time(nullptr));
  doc["type"] = type;

  char msg[512];
  serializeJson(doc, msg);
  bool sent = client.publish(topic_confirm, msg);
  if (sent) {
    Serial.println("Confirmation sent → " + pump_id + " | ID: " + String(command_id) + " | Tipo: " + type);
  } else {
    Serial.println("Error while sending MQTT confirmation");
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)payload[i];

  Serial.print("Payload: ");
  Serial.println(msg);

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, msg);
  if (err) {
    Serial.println("JSON not valid");
    return;
  }

  String topicStr = String(topic);

  if (topicStr == topic_command) {
    const char* pump_id = doc["pump_id"];
    const char* action = doc["action"];
    int command_id = doc["command_id"];

    if (String(action) != "start") return;

    String type = (command_id == -1) ? "auto" : "manual";

    if (String(pump_id) == "plant1") {
      Serial.println("Activating Pump 1");
      digitalWrite(RELAY1, LOW);
      delay(5000);
      digitalWrite(RELAY1, HIGH);
      time(&lastIrrigation1);
      sendConfirmation("plant1", command_id, type);
    } 
    else if (String(pump_id) == "plant2") {
      Serial.println("Activating Pump 2");
      digitalWrite(RELAY2, LOW);
      delay(5000);
      digitalWrite(RELAY2, HIGH);
      time(&lastIrrigation2);
      sendConfirmation("plant2", command_id, type);
    }
  }
}

String formatDateTime(time_t t) {
  struct tm* tm_info = localtime(&t);
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
  return String(buffer);
}