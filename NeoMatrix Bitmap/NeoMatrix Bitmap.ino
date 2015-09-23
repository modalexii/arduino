#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

#define PIN 6

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(8, 8, PIN,
NEO_MATRIX_TOP     + NEO_MATRIX_LEFT +
NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE,
NEO_GRB            + NEO_KHZ800);


int seeking = 54; // position of the color data on the bitmaps inside the sdcard DO NOT CHANGE
int r = 0;
int g = 0;  //initializing/resetting  all colors and all values
int b = 0;
int bitmapcolor = matrix.Color(0,0,0);


   int a[64];
   int d[64];
   int c[64];


void getem(char filepath[], int colorbuffer[64]){

File  thebmp = FileSystem.open(filepath,FILE_READ);  

if(thebmp){
  Serial.println("File opened");
}

  
 //int  colorbuffer[64]; // a temporary array where the converted rgb values will be stored
  int seeking = 51; //this is the starting point where the actual color data is stored anything before is simply header and properties info

   for (int x = 0 ; x < 64 ; x++){
        seeking = seeking + 3;// skip every 3 hex byte, essentially skip to next color
        thebmp.seek(seeking);
       
       
         if (thebmp.seek(seeking) == true)
         {Serial.println("seeking completed");}
     
         
         Serial.print("position on sd card :");
         Serial.println(seeking);
         Serial.println("Actual position : ");
         Serial.println(thebmp.position());

       
         for (int i = 1 ; i <= 3 ; i++){
         
           if (i = 1){
             b = thebmp.read();
           Serial.println(b);  
           }
         
            if (i = 2){
            g = thebmp.read();
            Serial.println(g);  
            }
         
            if (i = 3){
            r = thebmp.read();
         
            Serial.println(r);  
            Serial.print(r);
            Serial.print(",");
            Serial.print(g);
            Serial.print(",");
            Serial.print(b);
            bitmapcolor = matrix.Color(r,g,b); //this value acts as a temporary storage to then place in the other storage

        }
          }
          colorbuffer[x] = bitmapcolor;  //this then appends the bitmapcolor value in, if you read the values here they arnt standard RGB or HEX  

}

 thebmp.close();
 
}
void drawbitmap(int colorbuffer[] ){
 int bufferindex = 0;
  for (int y = 7 ; y >= 0 ; y--){ //after the color array is built, they are pushed into this loop where it build the each pixel 
    for (int x = 0 ; x < 8 ; x++){     //and displays according to the color array
        matrix.drawPixel(x,y,colorbuffer[bufferindex]);
        bufferindex++;
        
    }
 }
matrix.show();
}
void setup() {
  matrix.begin();
  matrix.setBrightness(200);
  matrix.fillScreen(0);
  Serial.begin(9600);
  Bridge.begin();
  FileSystem.begin();
  int seeking = 51;
  }

  void loop(){

    
    getem("FILEPATH",a);


    getem("Filepath",d);
    

    getem("Filepath",c);
    
    for (int i = 0; i < 20 ; i++){
      
      drawbitmap(a);
      delay(500);
      drawbitmap(d);
      delay(500);
      drawbitmap(c);
  delay(500);  
  }



}