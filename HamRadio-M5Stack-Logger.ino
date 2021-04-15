// https://github.com/m5stack/m5-docs/blob/master/docs/en/api/lcd.md

#include <M5Stack.h>
#include <WiFi.h>

#include "AudioFileSourceSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"

#include "FT857.h"
FT857 radio;

AudioGeneratorWAV *wav;
AudioFileSourceSD *file;
AudioOutputI2S *out;
AudioFileSourceID3 *id3;

unsigned int freq = 0;
unsigned int mode = 255;
unsigned int sMeter = 255;
bool radioPowerOn = false;
bool shouldRefresh = true;
bool testMode = true, contestMode = false, isTx = false;

void setup() {
  Serial.println("Starting ...");

  randomSeed(analogRead(0));
  
  out = new AudioOutputI2S(0, 1); // Output to builtInDAC
  out->SetOutputModeMono(true);
  out->SetGain(1);
  wav = new AudioGeneratorWAV();

  M5.begin();
  M5.Lcd.setBrightness(1);
  M5.Speaker.setVolume(1);
  M5.Speaker.update();
  
  M5.Lcd.setTextDatum(MC_DATUM);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(4);
  M5.Lcd.drawString("Starting", 160, 100);

  radio.begin();
  //checkRadio();
}

void loop() {  
  // update button state
  M5.update();
  
  if (M5.BtnC.wasPressed()) {
    testMode = !testMode;
    shouldRefresh = true;
  }
  
  if (M5.BtnA.wasPressed()) {
    contestMode = !contestMode;
    shouldRefresh = true;
  }

  bool newIsTx = radioPowerOn && radio.isTx();
  shouldRefresh |= newIsTx != isTx;
  isTx = newIsTx;

  if (M5.BtnB.wasPressed() && !wav->isRunning() && !isTx) {
    playAudioAndTx();
    shouldRefresh = true;
  }

  if (wav->isRunning()) {
    if (!wav->loop()) {
      wav->stop();
      
      if (radioPowerOn) {
        radio.setPtt(false);
      }
  
      delete id3;
      delete file;

      shouldRefresh = true;
    }
  }

  if (radioPowerOn) {
    unsigned int newFreq = (unsigned int) (radio.getFrequency() / 100);
    shouldRefresh |= newFreq != freq;
    
    int newMode = radio.getMode();
    shouldRefresh |= newMode != mode;
    
    int newSMeter = radio.getSMeter();
    shouldRefresh |= newSMeter != sMeter;
  
    if (shouldRefresh) {
      freq = newFreq;
      mode = newMode;
      sMeter = newSMeter;
      
      M5.Lcd.clear();
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setTextSize(3);
      M5.Lcd.drawString(String(freq / 1000.0, 3) + " MHz " + getStringMode(mode), 160, 0);
  
      if (isTx) {
        M5.Lcd.setTextColor(RED);
        M5.Lcd.setTextSize(5);
        if (wav->isRunning()) {
          M5.Lcd.drawString("Playing & TX !", 160, 100);
        } else {
          M5.Lcd.drawString("TX !", 160, 100);
        }
        M5.Lcd.setTextSize(3);
      } else {
        M5.Lcd.drawString("S-" + getStringSMeter(sMeter), 160, 40);
        M5.Lcd.progressBar(20, 60, 10, 10, sMeter / 15);
      }
    }
  } else if (shouldRefresh) {
    M5.Lcd.clear();
    M5.Lcd.setTextColor(RED);
    M5.Lcd.setTextSize(5);
    M5.Lcd.drawString("No radio !", 160, 100);
  }

  if (shouldRefresh) {
    if (wav->isRunning()) {
      M5.Lcd.setTextColor(GREEN);
      M5.Lcd.drawString("Playing !", 160, 150);
    }
  
    M5.Lcd.setTextSize(3);
    if (testMode) {
      M5.Lcd.setTextColor(YELLOW);
      M5.Lcd.drawString("Mode test", 0, 190);
    }
  
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.drawString("Mode " + String(contestMode ? "contest" : "normal"), 0, 220);
  }

  shouldRefresh = false;

  delay(10);
}

void playAudioAndTx() {  
  file = new AudioFileSourceSD(getFileToPlay());
  id3 = new AudioFileSourceID3(file);    

  if (radioPowerOn) {
    radio.setPtt(true);
    delay(1000);
  }

  wav->begin(id3, out);
}

bool checkRadio() {
  unsigned long timer = millis();
  radio.getMode();
  
  if (millis() - timer >= 3000) {
    Serial.println("Radio not ready !");
    M5.Lcd.clear();
    M5.Lcd.setTextColor(RED);
    M5.Lcd.setTextSize(5);
    M5.Lcd.drawString("No radio !", 160, 100);

    return false;
  }

  radioPowerOn = true;
  Serial.println("Radio ready !");
  return true;
}

const char* getFileToPlay() {
  if (testMode) {
    return random(0, 2) == 0 ? "/test.wav" : "/testing.wav";
  }
  
  if (contestMode) {
    return random(0, 2) == 0 ? "/cq-yota-contest-foxtrot.wav" : "/cq-yota-contest-foxtrot-florida-cq-contest.wav";
  }
  
  return random(0, 2) == 0 ? "/cq-florida-florida.wav" : "/cq-foxtrot-florida.wav";
}

String getStringMode(int mode) {
  switch (mode) {
    case CAT_MODE_LSB:
        return "LSB";

    case CAT_MODE_USB:
        return "USB";

    case CAT_MODE_FM:
        return "FM ";

    case CAT_MODE_FMN:
        return "FMN";
  }

  return "UNK";
}

String getStringSMeter(int sMeterValue) {
  if (sMeterValue < 10) {
    return String(sMeterValue);
  }
  else {
    switch (sMeterValue) {
        case 10:
            return "9+10";
  
        case 11:
            return "9+20";
  
        case 12:
            return "9+30";
  
        case 13:
            return "9+40";
  
        case 14:
            return "9+50";
  
        case 15:
            return "9+60";
  
        default:
            return "UNK";
    }
  }
}
