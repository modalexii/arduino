#include "LittleFS.h"
#include "ESPAsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "nRF24L01.h"
#include "RF24.h"
#include "Adafruit_NeoPixel.h"
#include "DNSServer.h"
#include "arduino-timer.h"
#include "wire.h"
#include "mcp23008.h"

/* RGB LED Indicator */
Adafruit_NeoPixel indicator = Adafruit_NeoPixel(1, 15, NEO_RGB + NEO_KHZ800);
uint32_t red = indicator.Color(100,0,0);
uint32_t green = indicator.Color(0,100,0);
uint32_t blue = indicator.Color(0,0,100);
uint32_t yellow = indicator.Color(100,85,0);
uint32_t magenta = indicator.Color(0,85,100);
Timer<1> indicator_sticky_timer;

void setIndicatorColor(uint32_t c, unsigned long duration) {
  indicator.setPixelColor(0, c); 
  indicator.show();
  if (duration > 0) {
    indicator_sticky_timer.cancel(); // clear tasks
    indicator_sticky_timer.in(duration, [](void*) -> bool { setIndicatorColor(green, 0); return false; }); // return to green
  }
}

/* MISC */
MCP23008 MCP(0x20); // GPIO Expander
DynamicJsonDocument config(1024); // Configuration, populated by loadConfig()

/* NRF24L01 Radio to Remotes */
RF24 radio(2,0); // CE, CSN 
const byte address[6] = "00001";
// transmission control
byte start_marker = 0x3C; // '<'
byte end_marker = 0x3E; // '>'
Timer<1> tx_sticky_timer;
bool tx_stuck; // whether or not we should be TXing
uint32_t transmission;

/* Web Services Dependencies */
const char *ssid = "swOmpFulcrum";
// const char *password = "testpassword";
const byte DNS_PORT = 53;
DNSServer dnsServer;
AsyncWebServer server(80);
uint8_t web_fire; // asme purpose as trigger_states, but for http fire invocation

/* Fulcrum Application Structures*/
struct fire_control {
  char name[16]; // friendly control name
  uint8_t trig_pin; // MCP expander pin
  uint8_t trig_index; // position in the OTA control signal and debounce list
  bool state; // current open/closed state
} triggers[] = {
  {"1", 0, 0, 0},
  {"2", 2, 1, 0},
  {"3", 4, 2, 0},
  {"4", 6, 3, 0}
};
// the 16 bytes of control data sent over the air, in which each bit represents
// a single relay state on the recieving end
uint16_t trigger_states;
uint16_t previous_trigger_states; // tracked exclusively for sane serial print debugging
uint16_t physical_states;
uint16_t mapped_physical_states;
uint16_t api_states;
//uint16_t dmx_states;

int control_count = sizeof(triggers)/sizeof(*triggers);

void setPinModes() {
  
  Wire.begin();
  MCP.begin();
  MCP.pinMode8(0x27);
  if (!MCP.isConnected()) {
    setIndicatorColor(red,50000);
    Serial.println("MCP is not connected! Physicals will not work.");
  }
  for (uint8_t i = 0; i < control_count; i++) {
    MCP.pinMode(triggers[i].trig_pin, INPUT_PULLUP);
  }
  
} // /setPinModes()

void collectPhysicalStats() {

  /*for (uint8_t i = 0; i < control_count; i++) { // all buttons
    bool changed = buttons[i].update();
    triggers[i].state = buttons[i].read();
    if (changed) { char cstr[16]; Serial.printf("button %s changed to %s\n", triggers[i].name, itoa(triggers[i].state,cstr,10)); }
    if (changed && triggers[i].state == LOW) { // pressed 
      setIndicatorColor(yellow);
      bitSet(physical_states, triggers[i].trig_index); // set trigger ON bit 
    } else if (triggers[i].state == HIGH) { // not pressed 
      bitClear(physical_states, triggers[i].trig_index); // set trigger OFF bit
    }
  }*/
  for (uint8_t i = 0; i < control_count; i++) { // all buttons
    triggers[i].state = MCP.digitalRead(triggers[i].trig_pin);
    if (triggers[i].state == LOW) { // pressed 
      setIndicatorColor(yellow, 250);
      bitSet(physical_states, triggers[i].trig_index); // set trigger ON bit 
    } else if (triggers[i].state == HIGH) { // not pressed 
      bitClear(physical_states, triggers[i].trig_index); // set trigger OFF bit
    }
  }

} // /collectPhysicalStats()

void coalesceControlStates() {
  // if the physical is engaged, map-set all bits specified in the config
  for (uint8_t i = 0; i < control_count; i++) { // each physical
    for (uint8_t b = 0; b < control_count; b++) { // each config bit
      if (config["levers"][i][b] == 1) { // if configured as mapped here
        if (bitRead(physical_states, triggers[i].trig_index) == 1) { // if physical is engaged
          bitSet(mapped_physical_states, b); //set corresponding bit
        } else { // physical not engaged
          bitClear(mapped_physical_states, b); //set corresponding bit
        }
      }
    }
  }
  // logical OR all possible sources of a fire directive
  trigger_states = mapped_physical_states | api_states; // | dmx_states
} // /coalesceControlStates

String processor(const String& var){
  // munged from https://github.com/me-no-dev/ESPAsyncWebServer#respond-with-content-coming-from-a-file-containing-templates
  // explicit substitutions
  if(var == "config_literal") {
    char json_string[300];
    serializeJson(config, json_string);
    return json_string;
  }

  // if none of the variables matched, see if theres a LittleFS file with the same name
  //Serial.printf("template processor: %s is not explicitly defined\n", var);
  if (LittleFS.exists(var)){
    File f = LittleFS.open(var, "r");
    if (f && f.size()) {
        String f_content;
        while (f.available()){
          f_content += char(f.read());
        }
        f.close();
        return f_content;
    }
  }
  // last report return empty string
  Serial.printf("template processor: %s is not defined, returning empty string\n", var);
  return String();
} // /processor

void loadConfig(bool use_defaults) {
  String file_name = use_defaults ? "config.default.json" : "config.json";
  File file = LittleFS.open(file_name, "r");
  if (!file) {
    Serial.printf("Could not open %s from LittleFS\n", file_name.c_str());
    if (!use_defaults) {
      loadConfig(true); // if we tried to load user config and failed, try default config 
      setIndicatorColor(red, 1000);
      return;
    }
  }
  DeserializationError error = deserializeJson(config, file);
  if (error) {
    Serial.printf("loadConfig on %s encountered DeserializationError: %s\n", file_name.c_str(), error.c_str());
    if (!use_defaults) {
      Serial.println("Retrying with default config to avoid total failure");
      setIndicatorColor(red, 1000);
      loadConfig(true); 
    } else {
      Serial.println("Could not load any config, nothing will work");
      setIndicatorColor(red, 100000);
      while(1) {}
    }
  }
  Serial.print("Loaded config: ");
  serializeJson(config, Serial);
  Serial.println();
} // /loadConfig

bool saveConfig() {
  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    setIndicatorColor(red, 5000);
    return false;
  }
  serializeJson(config, configFile);
  Serial.print("Saved config: ");
  serializeJson(config, Serial);
  Serial.println();
  return true;
} // /saveConfig

bool hasCorrrectPIN(AsyncWebServerRequest *req) {
  if (req->hasHeader("Cookie")) {
    AsyncWebHeader* h = req->getHeader("Cookie");
    String cookie_value = config["pin"];
    String cookie_field = "pin=";
    String cookie = cookie_field + cookie_value;
    if (h->value() == cookie) {
      return true;
    }
    Serial.printf("Incorrect PIN in cookie header: %s\n", h->value().c_str());
  }
  Serial.println("No cookie header");
  req->redirect("/");
  //setIndicatorColor(red, 1000);
  return false;
} // /hasCorrrectPIN
 
void instantiateWebApp() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    setIndicatorColor(blue, 500);
    Serial.printf("serving %s\n", request->url().c_str());
    request->send(LittleFS, "/index.htm", String(), false, processor);
  });

  server.on("/menu", HTTP_GET, [](AsyncWebServerRequest *request){
    setIndicatorColor(blue, 500);
    if (hasCorrrectPIN(request)) {
      request->send(LittleFS, "/menu.htm", String(), false, processor);
    }
  });

  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request){
    setIndicatorColor(blue, 500);
    if (hasCorrrectPIN(request)) {
      Serial.printf("serving %s\n", request->url().c_str());
      request->send(LittleFS, "/settings.htm", String(), false, processor);
    }
  });

  server.on("/fire", HTTP_GET, [](AsyncWebServerRequest *request){
    setIndicatorColor(blue, 500);
    if (hasCorrrectPIN(request)) {
      Serial.printf("serving %s\n", request->url().c_str());
      request->send(LittleFS, "/fire.htm", String(), false, processor);
    }
  });

  AsyncCallbackJsonWebHandler *settingsHandler = new AsyncCallbackJsonWebHandler("/settings", [](AsyncWebServerRequest *request, JsonVariant &json) {
    setIndicatorColor(blue, 500);
    DynamicJsonDocument data(1024);
    if (json.is<JsonArray>()) {
      data = json.as<JsonArray>();
    } else if (json.is<JsonObject>()) {
      data = json.as<JsonObject>();
    } 
    if (!data.containsKey("levers") || !data.containsKey("pin")) {
      setIndicatorColor(magenta, 1000);
      Serial.print("Invalid JSON body posted to /settings: ");
      serializeJson(data, Serial);
      Serial.println();
      request->send(500, "text/plain", "Invalid JSON ");
      return;
    }
    request->send(200);
    config = data;
    saveConfig();
    loadConfig(false);
  });
  server.addHandler(settingsHandler);

  AsyncCallbackJsonWebHandler *fireHandler = new AsyncCallbackJsonWebHandler("/fire", [](AsyncWebServerRequest *request, JsonVariant &json) {
    setIndicatorColor(yellow, 1000);
    if (hasCorrrectPIN(request)) {
      DynamicJsonDocument data(1024);
      if (json.is<JsonArray>()) {
        data = json.as<JsonArray>();
      } else if (json.is<JsonObject>()) {
        data = json.as<JsonObject>();
      } 
      Serial.print("JSON body posted to /fire: ");
      serializeJson(data, Serial);
      if (!data.containsKey("remotes")) {
        setIndicatorColor(magenta, 1000);
        Serial.print(" was invalid");
        serializeJson(data, Serial);
        Serial.println();
        request->send(500, "text/plain", "Invalid JSON");
        return;
      }
      Serial.println();
      request->send(200);
      for (uint8_t i = 0; i < control_count; i++) {
        if (data["remotes"][i] == 1) {
          bitSet(api_states, i);
        } else {
          bitClear(api_states, i);
        }
      }
    }
  });
  server.addHandler(fireHandler);

  server.serveStatic("/static", LittleFS, "/static/").setCacheControl("max-age=86400");
  server.serveStatic("/favicon.ico", LittleFS, "/static/favicon.ico").setCacheControl("max-age=86400");

  server.onNotFound([](AsyncWebServerRequest *request){
    setIndicatorColor(magenta, 500);
    Serial.printf("HTTP 404 %s\n", request->url().c_str());
    request->send(404, "text/html", "<h1>404</h1>");
  });
 
  server.begin();

} // /instantiateWebApp

void setup(){
  Serial.begin(115200);
  Serial.println(); // break away from boot serial dump

  indicator.begin();
  indicator.setBrightness(64);
  setIndicatorColor(yellow, 1000);

  if (!LittleFS.begin())  {
    Serial.println("An Error has occurred while mounting LittleFS");
    setIndicatorColor(red, 100000);
    while(1) {}
    return;
  }
  loadConfig(false);
 
  WiFi.softAP(ssid); //password);
  WiFi.hostname(ssid);
  Serial.print("WiFI IP address: ");
  Serial.println(WiFi.softAPIP());
  dnsServer.start(DNS_PORT, "fulcrum.lan", WiFi.softAPIP());

  instantiateWebApp();

  setPinModes();

  radio.begin();                  //Starting the Wireless communication
  radio.openWritingPipe(address); //Setting the address where we will send the data
  radio.setPALevel(RF24_PA_MAX);  //You can set it as minimum or maximum depending on the distance between the transmitter and receiver.
  radio.setDataRate(RF24_250KBPS);
  radio.stopListening();   

} // /setup

void TXTestBlocking() {
  // send a test pattern forever, when manually invoked
  setIndicatorColor(magenta, 0);
  Serial.println("Test mode");
  while (true) {
    for (uint8_t i=0; i<=3; i++) {
      trigger_states=0;
      bitSet(trigger_states, i);
      for (uint8_t i = 0; i<16; i++) {
        Serial.print(bitRead(trigger_states, i));
      }
      Serial.println();
      transmission = (start_marker << 24) |
        (highByte(trigger_states) << 16) |
        (lowByte(trigger_states) << 8) |
        end_marker;
      radio.write(&transmission, sizeof(transmission));
      delay(500); 
    } 
  }
} // /TXTestBlocking
 
void loop() {
  
  //TXTestBlocking();
  previous_trigger_states = trigger_states; 
  collectPhysicalStats();
  coalesceControlStates();

  // print trigger states to console on change
  if (trigger_states != previous_trigger_states) {
    Serial.print("New trigger states: ");
    for (uint8_t i = 0; i<16; i++) {
      Serial.print(bitRead(trigger_states, i));
    }
    Serial.println();
  }

  if (trigger_states != previous_trigger_states) {
    // our fire directive has changed - tell transmitter to spam the air for a while
    // doing this only for a fraction of a second prevents flapping when the bop-it is powered on
    tx_stuck = true;
    tx_sticky_timer.in(500, [](void*) -> bool { tx_stuck = false; return false; });
  }

  if (tx_stuck) { // wiggle the air
    transmission = (start_marker << 24) |
                  (highByte(trigger_states) << 16) |
                  (lowByte(trigger_states) << 8) |
                  end_marker;
    radio.write(&transmission, sizeof(transmission));
  }

  dnsServer.processNextRequest();

  indicator_sticky_timer.tick();

}