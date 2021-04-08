#include <M5Stack.h>
#include <WiFi.h>

#include "AudioFileSourceSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

#include "FT857.h"
FT857 radio;

AudioGeneratorMP3 *mp3;
AudioFileSourceSD *file;
AudioOutputI2S *out;
AudioFileSourceID3 *id3;

void setup() {
  Serial.println("Starting ...");
  
  M5.begin();
  delay(500);
  
  M5.Lcd.setTextFont(2);
  
  radio.begin();
  
  Serial.println("Radio Ready !");
  M5.Lcd.println("Radio Ready !");
  
  file = new AudioFileSourceSD("/hello.mp3");
  id3 = new AudioFileSourceID3(file);
  out = new AudioOutputI2S(0, 1); // Output to builtInDAC
  out->SetOutputModeMono(true);
  mp3 = new AudioGeneratorMP3();
  mp3->begin(id3, out);
}

void loop() {
  if (mp3->isRunning()) {
    if (!mp3->loop()) mp3->stop();
  } else {
    Serial.println(radio.getFrequency());
    M5.Lcd.println(radio.getFrequency());
    delay(1000);
  }
}
