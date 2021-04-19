// https://github.com/m5stack/m5-docs/blob/master/docs/en/api/lcd.md

#include <M5Stack.h>
#include <WiFi.h>
#include "AudioOutputI2S.h"
#include "AudioFileSourceSD.h"
#include "AudioGeneratorMP3.h"
#include "AudioGeneratorWAV.h"
#include "AudioFileSourceID3.h"
#include "FT857.h"

#define TEST 0
#define NORMAL 1
#define CONTEST 2

FT857 radio;

AudioGeneratorMP3 *player;
//AudioGeneratorWAV *player;
AudioFileSourceSD *file;
AudioOutputI2S *out;
AudioFileSourceID3 *id3;

bool radioPowerOn = true;
byte playerMode = NORMAL;
bool autoMode = false;
byte secondsBetweenPlaying = 5;

unsigned long freq = 0;
byte mode = 255;
byte sMeter = 255;
bool isTx = false;

bool shouldRefresh = true;

unsigned long timerRadioRefresh = 0;
unsigned long timerLoop = 0;

void setup() {
  Serial.println("Starting ...");

  randomSeed(analogRead(0));
  
  out = new AudioOutputI2S(0, 1);
  out->SetOutputModeMono(true);
  out->SetGain(1.0);
  player = new AudioGeneratorMP3();

  M5.begin();
  SD.begin();
  radio.begin();

  if (radioPowerOn) {
    if (!checkRadio()) {
      playerMode = TEST;
    }
  }
  
  M5.Lcd.setBrightness(100);
  M5.Lcd.setTextDatum(MC_DATUM);
}

void loop() {
  M5.update();
  radio.flushRX();

  bool newIsTx = radioPowerOn && radio.isTx();
  shouldRefresh |= newIsTx != isTx;
  isTx = newIsTx;

  if (isTx) {
    timerLoop = millis();
  }

  if (!player->isRunning() && !isTx) {
    if (autoMode && (millis() - timerLoop) >= 1000 * secondsBetweenPlaying) {
      playAudioAndTx();
      shouldRefresh = true;
    } else {
      if (M5.BtnB.wasReleased()) {
        autoMode = !autoMode;
        timerLoop = millis();
        shouldRefresh = true;
      } else if (M5.BtnC.wasReleased()) {
        secondsBetweenPlaying += 1;
        if (secondsBetweenPlaying > 15) {
          secondsBetweenPlaying = 5;
        }
        timerLoop = millis();
        shouldRefresh = true;
      } else if (M5.BtnA.pressedFor(1000, 1000)) {
        checkRadio();
        shouldRefresh = true;
      } else if (M5.BtnB.pressedFor(1000, 1000)) {
        playAudioAndTx();
        shouldRefresh = true;
      } else if (M5.BtnC.pressedFor(1000, 1000)) {
        playerMode++;
        playerMode %= 3;
        shouldRefresh = true;
      }
    }
  }

  if (M5.BtnA.wasPressed()) {
    stopAll(); 
    shouldRefresh = true;
  }

  if (stopPlayerAndRxIfNeeded(false)) {
    shouldRefresh = true;
  }

  if (radioPowerOn) {
    if (!player->isRunning() && !isTx && (millis() - timerRadioRefresh) >= 500) {
      radio.flushRX();
      unsigned long newFreq = (unsigned int) (radio.getFrequency() / 100);
      shouldRefresh |= newFreq != freq;
      
      byte newMode = radio.getMode();
      shouldRefresh |= newMode != mode;
      
      byte newSMeter = radio.getSMeter();
      shouldRefresh |= newSMeter != sMeter;
      
      freq = newFreq;
      mode = newMode;
      sMeter = newSMeter;
      
      timerRadioRefresh = millis();
    }
    
    if (shouldRefresh) {
      M5.Lcd.clear();
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setTextSize(3);
      M5.Lcd.drawString(String(freq / 1000.0, 3) + " MHz " + getStringMode(mode), 160, 0);
  
      if (isTx) {
        M5.Lcd.setTextColor(RED);
        M5.Lcd.setTextSize(5);
        M5.Lcd.drawString("TX", 160, 70);
      } else {
        M5.Lcd.drawString("S-" + getStringSMeter(sMeter), 160, 40);
        M5.Lcd.progressBar(20, 60, 10, 10, sMeter / 15);
      }
    }
  } else if (shouldRefresh) {
    M5.Lcd.clear();
    M5.Lcd.setTextColor(RED);
    M5.Lcd.setTextSize(3);
    M5.Lcd.drawString("No radio", 160, 0);
  }

  if (shouldRefresh) {
    if (player->isRunning()) {
      M5.Lcd.setTextColor(GREEN);
      M5.Lcd.setTextSize(5);
      M5.Lcd.drawString("Playing", 160, 130);
    }
  
    M5.Lcd.setTextSize(3);
    if (playerMode == TEST) {
      M5.Lcd.setTextColor(YELLOW);
      M5.Lcd.drawString("Mode test", 0, 220);
    } else {  
      M5.Lcd.setTextColor(GREEN);
      M5.Lcd.drawString("Mode " + String(playerMode == CONTEST ? "contest" : "normal"), 0, 220);
    }

    M5.Lcd.setTextColor(CYAN);
    M5.Lcd.drawString((!autoMode ? "No auto " : "Auto ") + String(secondsBetweenPlaying) + "s", 0, 190);
  }

  shouldRefresh = false;

  delay(10);
}

bool stopPlayerAndRxIfNeeded(bool force) {
  if (player->isRunning()) {
    if (!player->loop() || force) {
      player->stop();
      
      delete id3;
      delete file;

      rx();

      timerLoop = millis();
      
      return true;
    }
  }

  if (force) {
    rx();

    return true;
  }

  return false;
}

void tx() {
  if (radioPowerOn) {
    radio.setMode(CAT_MODE_DIG);
    radio.setPtt(true);
    radio.flushRX();
    isTx = radio.isTx();
    if (!isTx) {
      rx();
      return;
    }
    delay(300);
  }
}

void rx() {
  if (radioPowerOn && isTx) {
    delay(300);
    radio.flushRX();
    radio.setPtt(false);
    radio.flushRX();
    delay(300);
    radio.flushRX();
    radio.setMode(mode);
    isTx = radio.isTx();
  }
}

void stopAll() {
  stopPlayerAndRxIfNeeded(true);
  autoMode = false;
}

void playAudioAndTx() {
  if (!player->isRunning() && !isTx) {
    tx();
  
    file = new AudioFileSourceSD(getFileToPlay());
    id3 = new AudioFileSourceID3(file); 
    player->begin(id3, out);
  
    //if (
    M5.Speaker.mute();
  }
}

bool checkRadio() {
  M5.Lcd.clear();
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(5);
  M5.Lcd.drawString("Radio...", 160, 100);
    
  radioPowerOn = false;

  radio.flushRX();
  unsigned long timer = millis();
  byte mode = radio.getMode();
  
  if (millis() - timer >= 1000 || isModeUnknown(mode)) {
    return false;
  }

  radioPowerOn = true;
  return true;
}

const char* getFileToPlay() {
  switch (playerMode) {
    case TEST:
      return random(0, 2) == 0 ? "/testing.mp3" : "/test.mp3";

    case CONTEST:
      return random(0, 2) == 0 ? "/cq-yota-contest-foxtrot-contest.mp3" : "/cq-yota-contest-foxtrot-florida-cq-contest.mp3";

    case NORMAL:
    default:
      return random(0, 2) == 0 ? "/cq-florida-florida.mp3" : "/cq-foxtrot-florida.mp3";
  }
}

bool isModeUnknown(byte mode) {
  switch (mode) {
    case CAT_MODE_LSB:
      return false;

    case CAT_MODE_USB:
      return false;

    case CAT_MODE_FM:
      return false;

    case CAT_MODE_FMN:
      return false;

    case CAT_MODE_DIG:
      return false;
  }

  return true;
}

String getStringMode(byte mode) {
  switch (mode) {
    case CAT_MODE_LSB:
      return "LSB";

    case CAT_MODE_USB:
      return "USB";

    case CAT_MODE_DIG:
      return "DIG";

    case CAT_MODE_FM:
      return "FM ";

    case CAT_MODE_FMN:
      return "FMN";
  }

  return "UNK";
}

String getStringSMeter(byte sMeterValue) {
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
