#include <OctoWS2811.h>
#include <NewPing.h>


const int ledsPerStrip = 60;
DMAMEM int displayMemory[ledsPerStrip*6];
int drawingMemory[ledsPerStrip*6];
const int config = WS2811_GRB | WS2811_800kHz;
#define COLOR    0x005555
 
#define TRIGGER_PIN_A 3
#define ECHO_PIN_A 5
#define TRIGGER_PIN_B 6
#define ECHO_PIN_B 7
#define MAX_DISTANCE 200

OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);
NewPing sonar_a(TRIGGER_PIN_A, ECHO_PIN_A, MAX_DISTANCE);
NewPing sonar_b(TRIGGER_PIN_B, ECHO_PIN_B, MAX_DISTANCE);
 
void setup() {
   Serial.begin(9600);
   leds.begin();
   leds.show();
}

void partialFill(int color, int startLED, int endLED)
{
  for (int i=startLED; i < endLED; i++) {
    leds.setPixel(i, color);
  }
  leds.show();
}

void loop() {
  
   unsigned int p_a = sonar_a.ping_cm();
   unsigned int p_b = sonar_b.ping_cm();
   
   Serial.print("A: ");
   Serial.println(p_a);
   partialFill(0, 0, ledsPerStrip);
   partialFill(COLOR, 0, p_a/2);

   Serial.print("B: ");
   Serial.println(p_b);
   partialFill(0, ledsPerStrip, ledsPerStrip*2);
   partialFill(COLOR, ledsPerStrip, ledsPerStrip+p_b/2);
   
   delay(50);
}
