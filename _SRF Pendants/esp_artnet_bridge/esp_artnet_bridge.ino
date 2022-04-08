#include <ESP8266WiFi.h>         // https://github.com/esp8266/Arduino
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <WiFiClient.h>
#include <ArtnetWifi.h>          // https://github.com/rstephan/ArtnetWifi
#include <FS.h>
#include "FastLED.h"

#include "setup_ota.h"
#include "send_break.h"

#define NUM_LEDS 2
#define DATA_PIN 14
CRGB leds[NUM_LEDS];

#include <RH_ASK.h>
#ifdef RH_HAVE_HARDWARE_SPI
#include <SPI.h> // Not actually used but needed to compile
#endif
//H_ASK driver; // rx, tx, ptt
RH_ASK driver(2000, 4, 5, 0, true); // ESP8266 or ESP32: do not use pin 11 or 2

const unsigned long pattern_step_clk_period = 500; // how often (ms) to update the locally attached LEDs - no impact on TX function
const unsigned long tx_period = 5; // how often (ms) to transmit
uint8_t pattern_step = 0;
uint8_t pattern_step2 = 80;

#define MIN(x,y) (x<y ? x : y)
#define ENABLE_MDNS
#define ENABLE_WEBINTERFACE

#define FASTLED_ESP8266_NODEMCU_PIN_ORDER

Config config;
ESP8266WebServer server(80);
const char* host = "ARTNET"; // fallback ssid
const char* version = __DATE__ " / " __TIME__;

// Artnet settings
ArtnetWifi artnet;
unsigned long packetCounter = 0, frameCounter = 0;
float fps = 0;

// Global universe buffer
struct {
  uint16_t universe;
  uint16_t length;
  uint8_t sequence;
  uint8_t *data;
} global;

// keep track of the timing of the function calls
long tic_loop = 0, tic_fps = 0, tic_packet = 0, tic_web = 0;

//this will be called for each UDP packet received
void onDmxPacket(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {
  // print some feedback
  Serial.print("packetCounter = ");
  Serial.print(packetCounter++);
  if ((millis() - tic_fps) > 1000 && frameCounter > 100) {
    // don't estimate the FPS too frequently
    fps = 1000 * frameCounter / (millis() - tic_fps);
    tic_fps = millis();
    frameCounter = 0;
    Serial.print(",  FPS = ");
    Serial.print(fps);
  }
  Serial.print(" U=");
  Serial.print(universe);
  Serial.println();

  if (universe == config.universe) {
    // copy the data from the UDP packet over to the global universe buffer
    global.universe = universe;
    global.sequence = sequence;
    if (length < 512)
      global.length = length;
    for (int i = 0; i < global.length; i++)
      global.data[i] = data[i];

    leds[0].r = global.data[0];
    leds[0].g = global.data[1];
    leds[0].b = global.data[2];
    leds[1].r = global.data[3];
    leds[1].g = global.data[4];
    leds[1].b = global.data[5];
    Serial.print(global.data[0], HEX);
    Serial.print(global.data[1], HEX);
    Serial.print(global.data[2], HEX);
    Serial.print(global.data[3], HEX);
    Serial.print(global.data[4], HEX);
    Serial.println(global.data[5], HEX);
  }

} // onDmxpacket


void setup() {
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);

  Serial1.begin(250000, SERIAL_8N2);
  Serial.begin(115200);
  while (!Serial) {
    ;
  }
  Serial.println("setup starting");

  global.universe = 0;
  global.sequence = 0;
  global.length = 512;
  global.data = (uint8_t *)malloc(512);
  for (int i = 0; i < 512; i++)
    global.data[i] = 0;

  SPIFFS.begin();

  initialConfig();

  WiFiManager wifiManager;
  // wifiManager.resetSettings();
  // this stuff is for fallback AP mode if AP saved in EEPROM isnt available 
  WiFi.hostname(host);
  wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  wifiManager.autoConnect(host);
  wifiManager.setClass("invert");
  Serial.println("connected");

#ifdef ENABLE_WEBINTERFACE
  // this serves all URIs that can be resolved to a file on the SPIFFS filesystem
  server.onNotFound(handleNotFound);

  server.on("/", HTTP_GET, []() {
    tic_web = millis();
    handleRedirect("/index");
  });

  server.on("/index", HTTP_GET, []() {
    tic_web = millis();
    handleStaticFile("/index.html");
  });

  server.on("/defaults", HTTP_GET, []() {
    tic_web = millis();
    Serial.println("handleDefaults");
    handleStaticFile("/reload_success.html");
    delay(2000);
    initialConfig();
    saveConfig();
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    WiFi.hostname(host);
    ESP.restart();
  });

  server.on("/reconnect", HTTP_GET, []() {
    tic_web = millis();
    Serial.println("handleReconnect");
    handleStaticFile("/reload_success.html");
    delay(2000);
    WiFiManager wifiManager;
    wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
    wifiManager.startConfigPortal(host);
    Serial.println("connected");
  });

  server.on("/reset", HTTP_GET, []() {
    tic_web = millis();
    Serial.println("handleReset");
    handleStaticFile("/reload_success.html");
    delay(2000);
    ESP.restart();
  });

  server.on("/monitor", HTTP_GET, [] {
    tic_web = millis();
    handleStaticFile("/monitor.html");
  });

  server.on("/hello", HTTP_GET, [] {
    tic_web = millis();
    handleStaticFile("/hello.html");
  });

  server.on("/settings", HTTP_GET, [] {
    tic_web = millis();
    handleStaticFile("/settings.html");
  });

  server.on("/dir", HTTP_GET, [] {
    tic_web = millis();
    handleDirList();
  });

  server.on("/json", HTTP_PUT, [] {
    tic_web = millis();
    handleJSON();
  });

  server.on("/json", HTTP_POST, [] {
    tic_web = millis();
    handleJSON();
  });

  server.on("/json", HTTP_GET, [] {
    tic_web = millis();
    DynamicJsonDocument root(300);
    CONFIG_TO_JSON(universe, "universe");
    CONFIG_TO_JSON(channels, "channels");
    CONFIG_TO_JSON(delay, "delay");
    root["version"] = version;
    root["uptime"]  = long(millis() / 1000);
    root["packets"] = packetCounter;
    root["fps"]     = fps;
    String str;
    serializeJson(root, str);
    server.send(200, "application/json", str);
  });

  server.on("/update", HTTP_GET, [] {
    tic_web = millis();
    handleStaticFile("/update.html");
  });

  server.on("/update", HTTP_POST, handleUpdate1, handleUpdate2);

  // start the web server
  server.begin();
#endif

  // announce the hostname and web server through zeroconf
#ifdef ENABLE_MDNS
  MDNS.begin(host);
  MDNS.addService("http", "tcp", 80);
#endif

  artnet.begin();
  artnet.setArtDmxCallback(onDmxPacket);

  // initialize all timers
  tic_loop   = millis();
  tic_packet = millis();
  tic_fps    = millis();
  tic_web    = 0;

  // transmitter
  if (!driver.init()) {
    Serial.println("init failed");
  }
  // -----/og

  Serial.println("setup done");
} // setup

void spam() {
  uint8_t msg[9] = {
    byte(0x53), byte(0x52), byte(0x46), // "SRF"
    global.data[0], 
    global.data[1], 
    global.data[2], 
    global.data[3], 
    global.data[4],
    global.data[5]
  };
  driver.send((uint8_t *)msg, 9);
  driver.waitPacketSent();
}

void loop() {
  server.handleClient();

  if (WiFi.status() != WL_CONNECTED) {}
  else if ((millis() - tic_web) < 5000) {}
  else {
    artnet.read();
    // this section gets executed at a maximum rate of around 40Hz
    if ((millis() - tic_loop) > config.delay) {
      tic_loop = millis();
      frameCounter++;
    }
  }

  EVERY_N_MILLISECONDS(pattern_step_clk_period){
    FastLED.show();
  }

  EVERY_N_MILLISECONDS(tx_period){
    spam();
  }

} // loop 


