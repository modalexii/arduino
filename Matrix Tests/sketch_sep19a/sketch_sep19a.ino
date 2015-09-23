#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#ifndef PSTR
 #define PSTR // Make Arduino Due happy
#endif

#define SIZEOF_ARRAY(x)  (sizeof (x) / sizeof (x[0]) )

#define DATA_PIN 6

const int MATRIX_WIDTH = 18;
const int MATRIX_HEIGHT = 8;

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(
  MATRIX_WIDTH,
  MATRIX_HEIGHT,
  DATA_PIN,
  NEO_MATRIX_BOTTOM + NEO_MATRIX_LEFT +
  NEO_MATRIX_ROWS   + NEO_MATRIX_ZIGZAG,
  NEO_GRB           + NEO_KHZ800 
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

void setup(){
  matrix.begin();
  matrix.setBrightness(85);
  Serial.begin(9600);
}

void reverse(int a[], int sz) {
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

void advanceDisplay(int *matrix[], int amount) {
  for(int row = 0; row <= MATRIX_HEIGHT; row++) {
    rotate(matrix[row], MATRIX_WIDTH, amount);
  }
}

void loop(){
  matrix.fillScreen(0);
  for(int x = 0; x <= MATRIX_WIDTH; x++) {
    for(int y = 0; y <= MATRIX_HEIGHT; y++) {
      matrix.drawPixel(x,y,triangleBase[y][x]);
    }
  }
  matrix.show();
  
  delay(500);
  
  for(int row = 0; row <= MATRIX_HEIGHT; row++) {
    rotate(triangleBase[row], MATRIX_WIDTH, 1);
  }

}
