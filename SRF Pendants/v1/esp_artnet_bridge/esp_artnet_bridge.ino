#include <ESP8266WiFi.h>         // https://github.com/esp8266/Arduino
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <WiFiClient.h>
#include <ArtnetWifi.h>          // https://github.com/rstephan/ArtnetWifi
#include <FS.h>

#include "setup_ota.h"
#include "send_break.h"

//#include "FastLED.h"
#include <RH_ASK.h>
#ifdef RH_HAVE_HARDWARE_SPI
#include <SPI.h> // Not actually used but needed to compile
#endif
//H_ASK driver; // rx, tx, ptt
RH_ASK driver(2000, 4, 5, 0, true); // ESP8266 or ESP32: do not use pin 11 or 2
// RH_ASK driver(2000, 3, 4, 0); // ATTiny, RX on D3 (pin 2 on attiny85) TX on D4 (pin 3 on attiny85), 
// RH_ASK driver(2000, PD14, PD13, 0); STM32F4 Discovery: see tx and rx on Orange and Red LEDS

// og transmitter 
//const int led_pin = 2;
const int transmit_pin = 1;
const int receive_pin = 3;
const int transmit_en_pin = 5;

#define NUM_LEDS 1
#define DATA_PIN 6
//CRGB leds[NUM_LEDS];

unsigned long pattern_step_clk_start; 
unsigned long pattern_step_clk_now;
// controls speed of rainbow. mst be in sync with receiver
const unsigned long pattern_step_clk_period = 400;
uint8_t pattern_step = 0;
// -----/og

#define MIN(x,y) (x<y ? x : y)
#define ENABLE_MDNS
#define ENABLE_WEBINTERFACE

#define FASTLED_ESP8266_NODEMCU_PIN_ORDER
// #define COMMON_ANODE

Config config;
ESP8266WebServer server(80);
const char* host = "ARTNET";
const char* version = __DATE__ " / " __TIME__;

#define LED_B 16  // GPIO16/D0
#define LED_G 5   // GPIO05/D1
#define LED_R 4   // GPIO04/D2

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
  Serial.println();

  if (universe == config.universe) {
    // copy the data from the UDP packet over to the global universe buffer
    global.universe = universe;
    global.sequence = sequence;
    if (length < 512)
      global.length = length;
    for (int i = 0; i < global.length; i++)
      global.data[i] = data[i];
  }
} // onDmxpacket


void setup() {
  Serial1.begin(250000, SERIAL_8N2);
  Serial.begin(115200);
  while (!Serial) {
    ;
  }
  Serial.println("setup starting");

  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  global.universe = 0;
  global.sequence = 0;
  global.length = 512;
  global.data = (uint8_t *)malloc(512);
  for (int i = 0; i < 512; i++)
    global.data[i] = 0;

  SPIFFS.begin();

  initialConfig();

  if (loadConfig()) {
    singleYellow();
    delay(1000);
  }
  else {
    singleRed();
    delay(1000);
  }

  if (WiFi.status() != WL_CONNECTED)
    singleRed();

  WiFiManager wifiManager;
  // wifiManager.resetSettings();
  WiFi.hostname(host);
  wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  wifiManager.autoConnect(host);
  Serial.println("connected");

  if (WiFi.status() == WL_CONNECTED)
    singleGreen();

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
    singleRed();
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
    singleRed();
    WiFiManager wifiManager;
    wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
    wifiManager.startConfigPortal(host);
    Serial.println("connected");
    if (WiFi.status() == WL_CONNECTED)
      singleGreen();
  });

  server.on("/reset", HTTP_GET, []() {
    tic_web = millis();
    Serial.println("handleReset");
    handleStaticFile("/reload_success.html");
    delay(2000);
    singleRed();
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

  // og transmitter
  // Initialise the IO and ISR
  /*vw_set_tx_pin(transmit_pin);
  vw_set_rx_pin(receive_pin);
  vw_set_ptt_pin(transmit_en_pin);
  vw_set_ptt_inverted(true); // Required for DR3100
  vw_setup(2000);       // Bits per sec*/
  if (!driver.init()) {
    Serial.println("init failed");
  }
  //pinMode(led_pin, OUTPUT);
  pattern_step_clk_start = millis(); 
  //FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  // -----/og

  Serial.println("setup done");
} // setup

// og transmitter
/*
void display_hue(uint8_t hue) {
  leds[0] = CHSV( hue, 255, 160 );
  FastLED.show();
}*/

void spam() {
  uint8_t msg[4] = {byte(0x53), byte(0x52), byte(0x46), pattern_step};
  driver.send((uint8_t *)msg, 4);
  driver.waitPacketSent();
}

byte count = 1;
// -----/og

void loop() {
  server.handleClient();

  if (WiFi.status() != WL_CONNECTED) {
    singleRed();
    delay(10);
  }
  else if ((millis() - tic_web) < 5000) {
    singleBlue();
    delay(25);
  }
  else  {
    singleGreen();
    artnet.read();

    // this section gets executed at a maximum rate of around 40Hz
    if ((millis() - tic_loop) > config.delay) {
      tic_loop = millis();
      frameCounter++;

      sendBreak();

      Serial1.write(0); // Start-Byte
      // send out the value of the selected channels (up to 512)
      for (int i = 0; i < MIN(global.length, config.channels); i++) {
        Serial1.write(global.data[i]);
      }
    }
  }

  // og transmitter
  pattern_step_clk_now = millis(); 
  if (pattern_step_clk_now - pattern_step_clk_start >= pattern_step_clk_period) {
    Serial.println(pattern_step);
    //display_hue(pattern_step);
    pattern_step = pattern_step < 255 ? pattern_step + 1 : 0;
    pattern_step_clk_start = pattern_step_clk_now; // reset step timer 
  }
  spam();
  // -----/og

  delay(1);
} // loop


#ifdef COMMON_ANODE

void singleRed() {
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_B, HIGH);
}

void singleGreen() {
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, HIGH);
}

void singleBlue() {
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_B, LOW);
}

void singleYellow() {
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, HIGH);
}

void allBlack() {
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_B, HIGH);
}

#else

void singleRed() {
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, LOW);
}

void singleGreen() {
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_B, LOW);
}

void singleBlue() {
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, HIGH);
}

void singleYellow() {
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_B, LOW);
}

void allBlack() {
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, LOW);
}

#endif
