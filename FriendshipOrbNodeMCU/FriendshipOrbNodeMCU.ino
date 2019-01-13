#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_DotStar.h>
#include <SPI.h>    
#include <Encoder.h>

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#include <ESP8266mDNS.h>

#include <EEPROM.h>
#define EEPROM_SIZE 512

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

Encoder myEnc(D5, D4);    
long oldPosition  = -999; 

// The server for the setup portal
ESP8266WebServer server ( 80 );

// Update these with values suitable for your network.
const char* ssid = "....";
const char* password = ".....";
const char* mqtt_server = "68.183.121.10";

#define RESET_HOLD_PERIOD 3000
#define RESET_COLOR 0xFFFF00
unsigned long startedHoldingPeriod = 0;
bool holdingButton = false;

#define MQTT_PORT 1883

uint32_t currentColor = 0xFF00FF;
#define LIGHT_ON_DURATION 1000

#define INPUT_PIN D3

// Setup LEDs
#define NUMPIXELS 255 
#define DATAPIN    4
#define CLOCKPIN   5
Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DATAPIN, CLOCKPIN);

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

// Custom Parameters
#define CUSTOM_STRING_BUFFER_SIZE 40
char friendshipGroup[CUSTOM_STRING_BUFFER_SIZE] = "";
char orbName[CUSTOM_STRING_BUFFER_SIZE] = "";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
unsigned long lightStartedTime = 0;

WiFiManager wifiManager;
void setup() {
  Serial.begin(115200);

  EEPROM.begin(EEPROM_SIZE);

  for(int i = 0; i < CUSTOM_STRING_BUFFER_SIZE; i++)
  {
    friendshipGroup[i] = EEPROM.read(i);
  }
  for(int i = CUSTOM_STRING_BUFFER_SIZE; i < 2*CUSTOM_STRING_BUFFER_SIZE; i++)
  {
    orbName[i - CUSTOM_STRING_BUFFER_SIZE] = EEPROM.read(i);
  }

  WiFiManagerParameter custom_friend_group("Friendship Group", "Friendship Group", friendshipGroup, 40);
  WiFiManagerParameter orb_name_parameter("Orb Name", "Orb Name", orbName, 40);

  //add all your parameters here
  wifiManager.addParameter(&custom_friend_group);
  wifiManager.addParameter(&orb_name_parameter);

  
  pinMode(INPUT_PIN, INPUT_PULLUP);

  strip.begin(); // Initialize pins for output
  strip.show();  // Turn all LEDs off ASAP

  setup_wifi();


  strcpy(friendshipGroup, custom_friend_group.getValue());
  strcpy(orbName, orb_name_parameter.getValue());



  Serial.print("The Group: ");
  Serial.println(friendshipGroup);
  Serial.print("The Name: ");
  Serial.println(orbName);

  for(int i = 0; i < CUSTOM_STRING_BUFFER_SIZE; i++)
  {
    EEPROM.write(i, friendshipGroup[i]);
  }
  for(int i = CUSTOM_STRING_BUFFER_SIZE; i < 2*CUSTOM_STRING_BUFFER_SIZE; i++)
  {
    EEPROM.write(i, orbName[i - CUSTOM_STRING_BUFFER_SIZE]);
  }
  EEPROM.commit();

  client.setServer(mqtt_server, MQTT_PORT);
  client.setCallback(callback);
}




char htmlResponse[3000];

void handleRoot() {

  snprintf ( htmlResponse, 3000,
"<!DOCTYPE html>\
<html lang=\"en\">\
  <head>\
    <meta charset=\"utf-8\">\
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
  </head>\
  <body>\
          <h1>Time</h1>\
          <input type='text' name='date_hh' id='date_hh' size=2 autofocus> hh \
          <input type='text' name='date_mm' id='date_mm' size=2 autofocus> mm \
          <input type='text' name='date_ss' id='date_ss' size=2 autofocus> ss \
          <div>\
          <br><button id=\"save_button\">Save</button>\
          </div>\
    <script src=\"https://ajax.googleapis.com/ajax/libs/jquery/1.11.3/jquery.min.js\"></script>\    
    <script>\
      var hh;\
      var mm;\
      var ss;\
      $('#save_button').click(function(e){\
        e.preventDefault();\
        hh = $('#date_hh').val();\
        mm = $('#date_mm').val();\
        ss = $('#date_ss').val();\        
        $.get('/save?hh=' + hh + '&mm=' + mm + '&ss=' + ss, function(data){\
          console.log(data);\
        });\
      });\      
    </script>\
  </body>\
</html>"); 

   server.send ( 200, "text/html", htmlResponse );  

}


void handleSave() {
  if (server.arg("hh")!= ""){
    Serial.println("Hours: " + server.arg("hh"));
  }

  if (server.arg("mm")!= ""){
    Serial.println("Minutes: " + server.arg("mm"));
  }

  if (server.arg("ss")!= ""){
    Serial.println("Seconds: " + server.arg("ss"));
  }

}



void startDNS()
{
  if (!MDNS.begin("friendshipOrb")) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  // Start TCP (HTTP) server
  server.on ( "/", handleRoot );
  server.on ("/save", handleSave);

  server.begin();
  Serial.println("TCP server started");

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
}

void runDNS()
{
  MDNS.update();
  server.handleClient();
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("About to Setup Orb");

  lightResetPixelColor();
  strip.show();
  //wifiManager.resetSettings();
  wifiManager.autoConnect("Friendship Orb");

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  startDNS();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  myEnc.write(payload[0]);

  lightStartedTime = millis();

}

void reconnect() {

  setup_wifi();

  // Loop until we're reconnected
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(orbName)) {
      Serial.println("connected");
      client.subscribe(friendshipGroup);
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

void clearAllPixels()
{
  for(int i = 0; i < NUMPIXELS; i++)
  {
    strip.setPixelColor(i, 0x000000);
  }
}

void lightMainPixelColor()
{
  for(int i = 0; i < NUMPIXELS; i++)
  {
    strip.setPixelColor(i, currentColor);
  }
}

void lightResetPixelColor()
{
  for(int i = 0; i < NUMPIXELS; i++)
  {
    strip.setPixelColor(i, RESET_COLOR);
  }
}

void doYieldDelay(unsigned long length)
{
  unsigned long startTime = millis();

  while(millis() - startTime < length)
  {
    yield();
  }
}

void doWiFiReset()
{
  for(int i = 0; i < 3; i++)
  {
    lightResetPixelColor();
    strip.show();
    delay(200);
    clearAllPixels();
    strip.show();
    delay(200);
  }

  Serial.println("About to disconnect and reset");
  WiFi.disconnect(true);
  doYieldDelay(2000);
  ESP.reset();
  doYieldDelay(2000);

}

void loop() {

  runDNS();

  long newPosition = myEnc.read();
  if (newPosition != oldPosition) {
    oldPosition = newPosition;
    Serial.println(newPosition);
  }
  currentColor = Wheel(newPosition%255);

  if (!client.connected()) 
  {
    reconnect();
  }
  client.loop();

  if(digitalRead(INPUT_PIN) == 0)
  {
    msg[0] = (char)newPosition%255;
    client.publish(friendshipGroup, msg);

    if(!holdingButton)
    {
      holdingButton = true;
      startedHoldingPeriod = millis();
    }
  }
  else
  {
    holdingButton = false;
  }

  if( holdingButton && ((millis() - startedHoldingPeriod) > RESET_HOLD_PERIOD) )
  {
    doWiFiReset();
  }

  //if(millis() - lightStartedTime < LIGHT_ON_DURATION)
  //{
    lightMainPixelColor();
  //}
  //else
  //{
    //clearAllPixels();
  //}
  strip.show();
}