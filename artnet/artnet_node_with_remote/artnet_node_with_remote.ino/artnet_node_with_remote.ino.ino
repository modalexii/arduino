#include <Artnet.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>
#include <Adafruit_NeoPixel.h>


#define W5200_CS  10
#define SDCARD_CS 4

// NeoPixel settings
const int numLeds = 81;
const int numberOfChannels = numLeds * 3; // (1 led = 3 channels)
const byte dataPin = 6;
Adafruit_NeoPixel leds = Adafruit_NeoPixel(numLeds, dataPin, NEO_GRB + NEO_KHZ800);

// ArtNet Recieve
Artnet artnet;
const int rcvStartUniverse = 0; 
const int maxUniverses = numberOfChannels / 512 + ((numberOfChannels % 512) ? 1 : 0);
bool universesReceived[maxUniverses];
bool sendFrame = 1;
int previousDataLength = 0;

// ArtNet Remote Control
byte serverIP[] = {10,0,0,56};
unsigned int artNetPort = 6454;
const int remoteUniverse = 0;
const int numChannels=512;
char ANProtoHead[8]="Art-Net";
const int ANProtoHeaderSize=17;
short OpOutput= 0x5000;
byte DmxBuffer[numChannels]; //buffer used for Dmx data
EthernetUDP Udp;
byte  ArtDmxBuffer[(ANProtoHeaderSize+numChannels)+8+1];

// Networking
//byte ip[] = {192, 168, 1, 175};
//byte mac[] = {0x22, 0x09, 0x05, 0x20, 0x69, 0x2C};
byte ip[] = {10,0,0,12};
byte mac[] = {0x04, 0xE9, 0xEE, 0x00, 0x69, 0xEC};

void setup() {
  
  Serial.begin(9600);
  
  pinMode(SDCARD_CS,OUTPUT);
  digitalWrite(SDCARD_CS,HIGH);//Deselect the SD card
  pinMode(W5200_CS,OUTPUT);
  digitalWrite(W5200_CS,LOW);
  
  pinMode(10,OUTPUT);
  artnet.begin(mac, ip);
  leds.begin();
  initTest();

  // this will be called for each packet received
  artnet.setArtDmxCallback(onDmxFrame);

  // ArtNet Remote Control
  initilizeANHeader();
  Ethernet.begin(mac,ip);
  Udp.begin(artNetPort);
  
}

void loop() {
  
  // Recieve ArtNet
  artnet.read();

  // ArtNet Remote
   check_arduino_inputs();
   initilizeANHeader(); // does this need to be done twice??
   Udp.beginPacket(serverIP, artNetPort);
   Udp.write(ArtDmxBuffer,(ANProtoHeaderSize+numChannels+1)); // was Udp.sendPacket
   Udp.endPacket();
   
}

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {
  
  Serial.print(millis());
  Serial.println(" DMX Packet recieved");
  sendFrame = 1;
  // set brightness of the whole strip 
  if (universe == 15)
  {
    leds.setBrightness(data[0]);
    leds.show();
  }

  // Store which universe has got in
  if ((universe - rcvStartUniverse) < maxUniverses)
    universesReceived[universe - rcvStartUniverse] = 1;

  for (int i = 0 ; i < maxUniverses ; i++)
  {
    if (universesReceived[i] == 0)
    {
      Serial.println("Broke");
      sendFrame = 0;
      break;
    }
  }

  // read universe and put into the right part of the display buffer
  for (int i = 0; i < length / 3; i++)
  {
    int led = i + (universe - rcvStartUniverse) * (previousDataLength / 3);
    if (led < numLeds)
      leds.setPixelColor(led, data[i * 3], data[i * 3 + 1], data[i * 3 + 2]);
  }
  previousDataLength = length;     
  
  if (sendFrame)
  {
    leds.show();
    // Reset universeReceived to 0
    memset(universesReceived, 0, maxUniverses);
  }
}

void initTest() {

  // Briefly show primary colors to confirm basic controller functionality
  // & strip power/data connections

  int delayTime = 500;
  
  for (int i = 0 ; i < numLeds ; i++)
    leds.setPixelColor(i, 127, 0, 0);
  leds.show();
  delay(delayTime);
  for (int i = 0 ; i < numLeds ; i++)
    leds.setPixelColor(i, 0, 127, 0);
  leds.show();
  delay(delayTime);
  for (int i = 0 ; i < numLeds ; i++)
    leds.setPixelColor(i, 0, 0, 127);
  leds.show();
  delay(delayTime);
  for (int i = 0 ; i < numLeds ; i++)
    leds.setPixelColor(i, 0, 0, 0);
  leds.show();
  
}

void check_arduino_inputs() {
  
 // FOR TESTING REMOTE CONTORL
 // TO BE DELETED

  int temp_val=0;
  for(int i=0;i<6;i++)
  //reads the 6 analogic inputs and set the data from 1023 steps to 255 steps (dmx)
  {
    temp_val=analogRead(i); 
    DmxBuffer[i]=byte(temp_val/4);
  }
  
}

void initilizeANHeader() {
  
  for (int i=0;i<7;i++) {
    ArtDmxBuffer[i]=ANProtoHead[i];
  }   
  
  //Operator code low byte first  
  ArtDmxBuffer[8]=OpOutput;
  ArtDmxBuffer[9]= OpOutput >> 8;
  ArtDmxBuffer[10]=0; // protocol
  ArtDmxBuffer[11]=14;
  ArtDmxBuffer[12]=0; // sequence
  ArtDmxBuffer[13] = 0; // physical
  ArtDmxBuffer[14]= remoteUniverse; // universe
  ArtDmxBuffer[15]= remoteUniverse>> 8;
  ArtDmxBuffer[16] = numChannels>> 8; // data length
  ArtDmxBuffer[17] = numChannels;

  for (int t= 0;t<numChannels;t++) {
    ArtDmxBuffer[t+ANProtoHeaderSize+1]=DmxBuffer[t];    
  }
     
}

