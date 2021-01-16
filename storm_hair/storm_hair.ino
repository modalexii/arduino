int mic = A3;
int pot = A1;
int led_a = 4;
int led_b = 1;
uint8_t peak = 0;
uint8_t luma_a = 0;
uint8_t luma_b = 0;
uint8_t luma_max = 255;
uint8_t luma_high = 180;
uint8_t luma_mid = 80;
uint8_t luma_low = 20;
const uint8_t peak_lumas_4 [] = {luma_high, luma_high, luma_max, luma_max };
const uint8_t peak_lumas_3 [] = {0, luma_low, luma_mid, luma_high};
const uint8_t peak_lumas_2 [] = {0, 0, 0, luma_mid};
const uint8_t peak_lumas_1 [] = {0, 0, 0, luma_low};
int trig_threshold = 300;

unsigned long pattern_clk_start; 
unsigned long pattern_clk_now;
const unsigned long pattern_clk_period = 10; 

unsigned long peak_spacing_clk_start; 
unsigned long peak_spacing_clk_now;
const unsigned long peak_spacing_clk_period = 350; 

// Variables to find the peak-to-peak amplitude of AUD output
const int sampleTime = 50; 
int micOut;

void setup() {
  //Serial.begin(9600);
  pinMode(mic, INPUT);
  pinMode(pot, INPUT);
  pinMode(led_a, OUTPUT);
  pinMode(led_b, OUTPUT);
  pattern_clk_start = millis();
  peak_spacing_clk_start = millis();
  randomSeed(analogRead(mic));
}

void loop() {
  trig_threshold = analogRead(pot);
  pattern_clk_now = millis();
  peak_spacing_clk_now = millis();
  int audio_level = findPTPAmp();
  if (audio_level > trig_threshold) {
    //Serial.println("Firing");
    peak = 4;
    pattern_clk_start = pattern_clk_period + 1; // bump timer so animation starts now
  }
  if (pattern_clk_now - pattern_clk_start >= pattern_clk_period) {
    // time for a pattern animation frame
    draw_frame();
    pattern_clk_start = pattern_clk_now; // reset pattern timer
  }
  if (peak_spacing_clk_now - peak_spacing_clk_start >= peak_spacing_clk_period) {
    // time for the next peak, if we're in the middle of an animation
    if ( peak > 0 ) { Serial.print("Peak: "); Serial.println(peak); }
    peak = max(0, peak - 1);
    peak_spacing_clk_start = peak_spacing_clk_now; // reset pattern timer
  }
}   

void draw_frame() {
  switch (peak) {
    case 4:
      luma_a = peak_lumas_4[random(0,3)];
      luma_b = peak_lumas_4[random(0,3)];
      break;
    case 3:
      luma_a = peak_lumas_3[random(0,3)];
      luma_b = peak_lumas_3[random(0,3)];
      break;
    case 2:
      luma_a = peak_lumas_2[random(0,3)];
      luma_b = peak_lumas_2[random(0,3)];
      break;
    case 1:
      if (luma_a > 0) {
        luma_a = peak_lumas_1[random(0,3)];
      }
      if (luma_b > 0) {
        luma_b = peak_lumas_1[random(0,3)];
      }
      break;
  }
  /*Serial.print("luma a: ");
  Serial.print(luma_a);
  Serial.print(" b:");
  Serial.println(luma_b);*/
  analogWrite(led_a, luma_a);
  analogWrite(led_b, luma_b);
  luma_a = max(0, luma_a-(luma_a/3));
  luma_b = max(0, luma_b-(luma_b/3));
}

// Find the Peak-to-Peak Amplitude Function
int findPTPAmp(){
// Time variables to find the peak-to-peak amplitude
   unsigned long startTime= millis();  // Start of sample window
   unsigned int PTPAmp = 0; 

// Signal variables to find the peak-to-peak amplitude
   unsigned int maxAmp = 0;
   unsigned int minAmp = 1023;

// Find the max and min of the mic output within the 50 ms timeframe
   while(millis() - startTime < sampleTime) 
   {
      micOut = analogRead(mic);
      if( micOut < 1023) //prevent erroneous readings
      {
        if (micOut > maxAmp)
        {
          maxAmp = micOut; //save only the max reading
        }
        else if (micOut < minAmp)
        {
          minAmp = micOut; //save only the min reading
        }
      }
   }

  PTPAmp = maxAmp - minAmp; // (max amp) - (min amp) = peak-to-peak amplitude
  double micOut_Volts = (PTPAmp * 3.3) / 1024; // Convert ADC into voltage

  //Uncomment this line for help debugging (be sure to also comment out the VUMeter function)
  //Serial.println(PTPAmp); 

  //Return the PTP amplitude to use in the soundLevel function. 
  // You can also return the micOut_Volts if you prefer to use the voltage level.
  return PTPAmp;   
}
