#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#ifndef PSTR
 #define PSTR // Make Arduino Due happy
#endif

#define SIZEOF_ARRAY(x)  (sizeof (x) / sizeof (x[0]) )

#define DATA_PIN 6
#define PIXEL_COUNT 144 // only for rainbow cycle mode

#define MATRIX_WIDTH 18
#define MATRIX_HEIGHT 8
//const int MATRIX_WIDTH = 18;
//const int MATRIX_HEIGHT = 8;
const int MATRIX_BRIGHTNESS = 20;


Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(
  MATRIX_WIDTH,
  MATRIX_HEIGHT,
  DATA_PIN,
  NEO_MATRIX_BOTTOM + NEO_MATRIX_LEFT +
  NEO_MATRIX_ROWS   + NEO_MATRIX_ZIGZAG,
  NEO_GRB           + NEO_KHZ800 
);

Adafruit_NeoPixel strip = Adafruit_NeoPixel(
  // only for rainbow cycle mode 
  PIXEL_COUNT,
  DATA_PIN,
  NEO_GRB + NEO_KHZ800
);

int triangleBase[8][18] = {
	{2016,2016,2016,2016,2016,63519,2016,2016,2016,2016,2016,63519,2016,2016,2016,2016,2016,63519},
	{2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016},
	{2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016},
	{2016,2016,63519,2016,2016,2016,2016,2016,63519,2016,2016,2016,2016,2016,63519,2016,2016,2016},
	{2016,2016,63519,2016,2016,2016,2016,2016,63519,2016,2016,2016,2016,2016,63519,2016,2016,2016},
	{2016,63519,63519,63519,2016,2016,2016,63519,63519,63519,2016,2016,2016,63519,63519,63519,2016,2016},
	{2016,63519,63519,63519,2016,2016,2016,63519,63519,63519,2016,2016,2016,63519,63519,63519,2016,2016},
	{63519,63519,63519,63519,63519,2016,63519,63519,63519,63519,63519,2016,63519,63519,63519,63519,63519,2016}
};

int stripeBase1[8][18] = {
	{1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616},
	{63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864},
	{64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303},
	{1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616},
	{63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864},
	{64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303},
	{1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616},
	{63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864}
};

int highlighterBase[8][18] = {
  {63506,63506,63506,64963,64963,61314,61314,2025,2025,1851,1851,43423,0,0,0,0,0,0},
  {0,63506,63506,64963,64963,61314,61314,2025,2025,1851,1851,43423,43423,0,0,0,0,0},
  {0,0,63506,63506,64963,64963,61314,61314,2025,2025,1851,1851,43423,43423,0,0,0,0},
  {0,0,63506,63506,64963,64963,61314,61314,2025,2025,1851,1851,43423,43423,0,0,0,0},
  {0,0,0,63506,63506,64963,64963,61314,61314,2025,2025,1851,1851,43423,43423,0,0,0},
  {0,0,0,63506,63506,64963,64963,61314,61314,2025,2025,1851,1851,43423,43423,0,0,0},
  {0,0,0,0,63506,63506,64963,64963,61314,61314,2025,2025,1851,1851,43423,43423,0,0},
  {0,0,0,0,0,63506,64963,64963,61314,61314,2025,2025,1851,1851,43423,43423,43423,0}
};

void setup(){

  strip.begin(); // only for rainbow cycle mode 
  strip.setBrightness(65);
  
  matrix.begin();
  matrix.setBrightness(70);
  
  pinMode(A1, INPUT_PULLUP);
  digitalWrite(A1, HIGH);
  pinMode(A2, INPUT_PULLUP);
  digitalWrite(A2, HIGH);
  pinMode(A3, INPUT_PULLUP);
  digitalWrite(A3, HIGH);
  pinMode(A4, INPUT_PULLUP);
  digitalWrite(A4, HIGH);
  pinMode(A5, INPUT_PULLUP);
  digitalWrite(A5, HIGH);
  
  pinMode(A6, INPUT);
  digitalWrite(A6, LOW);
  
  //Serial.begin(9600);
  
}

void drawScreen(int pixelArray[][MATRIX_WIDTH]) {

  // given a 2d array representing the display, show it

  for(int x = 0; x <= MATRIX_WIDTH; x++) {
    for(int y = 0; y <= MATRIX_HEIGHT; y++) {
      matrix.drawPixel(x,y,pixelArray[y][x]);
    }
  }

  matrix.show();

}

void reverse(int a[], int sz) {
  // supports rotate()
  int i, j;
  for (i = 0, j = sz; i < j; i++, j--) {
    int tmp = a[i];
    a[i] = a[j];
    a[j] = tmp;
  }
}

void rotate(int array[], int size, int amt) {
  
  if (amt < 0)
    amt = size + amt;
  reverse(array, size-amt-1);
  reverse(array+size-amt, amt-1);
  reverse(array, size-1);
  
}

uint32_t Wheel(byte WheelPos) {
  
  // Input a value 0 to 255 to get a color value.
  // The colours are a transition r - g - b - back to r.
  
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

void rainbowCycle(uint8_t rpt) {

  strip.show(); // only for rainbow cycle mode 
  
  uint16_t i, j;

  for(j = 0; j < 256 * rpt; j++) { 
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel( ( (i * 256 / strip.numPixels() ) + j) & 255) );
    }
    strip.show();
    delay(20);
  }

}

void marquee(int pixelArray[][MATRIX_WIDTH], int rpt, int wait) {
  for(int iter = 0; iter < rpt; iter++) {

    drawScreen(pixelArray);

    delay(wait);
    
    for(int row = 0; row <= MATRIX_HEIGHT; row++) {
      rotate(pixelArray[row], 18, 1);
    }
  
  }
}

void marqueeR(int pixelArray[][MATRIX_WIDTH], int rpt, int wait) {
  for(int iter = 0; iter < rpt; iter++) {

    drawScreen(pixelArray);

    delay(wait);
    
    for(int row = 0; row <= MATRIX_HEIGHT; row++) {
      rotate(pixelArray[row], 18, -1);
    }
  
  }
}



void stripeMarquee1(int rpt) {
  for(int iter = 0; iter < rpt; iter++) {
    
    drawScreen(stripeBase1);

    delay(100);
    
    for(int row = 0; row <= MATRIX_HEIGHT; row++) {
      rotate(stripeBase1[row], MATRIX_WIDTH, -1);
    }
    
  }
  for(int iter = 0; iter < rpt; iter++) {
    
    drawScreen(stripeBase1);

    delay(100);
    
    for(int row = 0; row <= MATRIX_HEIGHT; row++) {
      rotate(stripeBase1[row], MATRIX_WIDTH, 1);
    }
    
  }
}
 /*
void stripes() {

  int a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r = 0;
  int template[8][18] = {
    {a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r},
    {a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r},
    {a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r},
    {a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r},
    {a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r},
    {a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r},
    {a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r},
    {a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r}
  };

  a = 
  
}
*/

void loop(){

  if(digitalRead(A1) == LOW) {
    rainbowCycle(5);
  }
  if(digitalRead(A2) == LOW) {
    marquee(highlighterBase,MATRIX_WIDTH*5,110);
    marqueeR(highlighterBase,MATRIX_WIDTH*4,15);
    marquee(highlighterBase,MATRIX_WIDTH*5,110);
    marqueeR(highlighterBase,MATRIX_WIDTH*4,15);
  }
  if(digitalRead(A3) == LOW) {
    stripeMarquee1(30);
  }
  if(digitalRead(A4) == LOW) {
     //
  }
  if(digitalRead(A5) == LOW) {
     //
  }

}
