
#define playThreshold 100 
#define readCount 500
#define musicPin A0

void setup() {
  pinMode(musicPin,INPUT);
  Serial.begin(115200);
}

bool musicIsPlaying() {

  int sum = 0;

  for (uint8_t i=0; i<readCount; i++) {
    sum += abs(analogRead(musicPin));
    //Serial.print(sum);
    //Serial.print(",");
  }
 
  Serial.println(sum);
  return sum > playThreshold ? true : false;
  
}

void loop() {
  Serial.println("1");
  bool musicPlaying = musicIsPlaying();
  Serial.print("music playing: ");
  Serial.println(musicPlaying);
  delay(500);
}
