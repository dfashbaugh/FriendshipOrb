#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.
//const char* ssid = "........";
//const char* password = "........";
//const char* mqtt_server = "broker.mqtt-dashboard.com";

const char* ssid = "MagDylan";
const char* password = "ClarkIsACat";
const char* mqtt_server = "0.tcp.ngrok.io";
#define MQTT_PORT 14708

#define IN_TOPIC "friendship"
#define OUT_TOPIC "friendship"

#define INPUT_PIN 14

#define LIGHT_COLOR 0xFF00FF
#define LIGHT_ON_DURATION 1000

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
unsigned long lightStartedTime = 0;

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, MQTT_PORT);
  client.setCallback(callback);

  pinMode(INPUT_PIN, INPUT_PULLUP);
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  lightStartedTime = millis();

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      client.subscribe(IN_TOPIC);
    } 
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {

  if (!client.connected()) 
  {
    reconnect();
  }
  client.loop();

  if(digitalRead(INPUT_PIN) == 0)
  {
    client.publish(OUT_TOPIC, msg);
  }

  if(millis() - lightStartedTime < LIGHT_ON_DURATION)
  {
    // Light the light
  }
  else
  {
    // Turn off the Light
  }
}