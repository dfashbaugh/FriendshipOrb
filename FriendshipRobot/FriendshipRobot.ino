#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Servo.h>

Servo myservo;  

static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;
static const uint8_t D9   = 3;
static const uint8_t D10  = 1;

// Update these with values suitable for your network.
const char* ssid = "....";
const char* password = ".....";
const char* mqtt_server = "192.168.0.161";

#define MQTT_PORT 1883

#define MQTT_CLIENT_NAME "Smash"

#define MOVEMENT_DURATION 1000

#define IN_TOPIC "friendship"
#define OUT_TOPIC "friendship"

#define INPUT_PIN D3
#define SERVO_PIN D5

#define ON_POSITION 140
#define OFF_POSITION 30
#define SPEED_PERIOD 10

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
unsigned long messageReceivedTime = 0;

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, MQTT_PORT);
  client.setCallback(callback);

  pinMode(INPUT_PIN, INPUT_PULLUP);
  myservo.attach(SERVO_PIN); 
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

  messageReceivedTime = millis();

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(MQTT_CLIENT_NAME)) {
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

void returnToDefaultPosition()
{
  myservo.write(OFF_POSITION);
}

void goToTriggeredPosition()
{
  myservo.write(ON_POSITION); 
}

void loop() {

  if (!client.connected()) 
  {
    reconnect();
  }
  client.loop();

  if(digitalRead(INPUT_PIN) == 0)
  {
    msg[0] = 128; // Just send a random color for now to the color based Friendship Devices
    client.publish(OUT_TOPIC, msg);
  }

  if(millis() - messageReceivedTime < MOVEMENT_DURATION)
  {
    goToTriggeredPosition();
  }
  else
  {
    returnToDefaultPosition();
  }
}