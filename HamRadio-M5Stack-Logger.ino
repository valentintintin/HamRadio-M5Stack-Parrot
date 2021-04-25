// https://github.com/m5stack/m5-docs/blob/master/docs/en/api/lcd.md

#include <M5Stack.h>
#include <WiFi.h>
#include <Preferences.h>
#include "AudioOutputI2S.h"
#include "AudioFileSourceSD.h"
#include "AudioGeneratorMP3.h"
#include "AudioGeneratorWAV.h"
#include "AudioFileSourceID3.h"
#include "FT857.h"

#define SPE Serial.print(F(" "));
#define SLN Serial.println();

#define TIME_BETWEEN_REFRESH_RADIO 350

#define TEST 0
#define NORMAL 1
#define CONTEST 2

Preferences prefs;
FT857 radio;

// Use 8000 Hz sampling rate and 16 bits file encoding
AudioGeneratorMP3 *player;
//AudioGeneratorWAV *player;
AudioFileSourceSD *file;
AudioOutputI2S *out;
AudioFileSourceID3 *id3;

#define MAX_MP3_DIRECTORIES 10
#define MAX_LENGHT_MP3_DIRECTORY 16
#define MAX_MP3_IN_DIRECTORY 3
#define MAX_MP3_FILE_NAME 255

struct Mp3 {
  char name[MAX_LENGHT_MP3_DIRECTORY];
  char files[MAX_MP3_IN_DIRECTORY][MAX_MP3_FILE_NAME + MAX_LENGHT_MP3_DIRECTORY + 4]; // 4 = / + one more / and safety
  byte nbFiles;
};

struct Mp3 mp3Directories[MAX_MP3_DIRECTORIES];
byte nbMp3Directories = 0;

bool radioPowerOn;
bool useSd;
byte playerMode;
bool autoMode;
byte secondsBetweenPlaying;

unsigned long freq = 0;
byte mode = 255;
byte sMeter = 255;
bool isTx = false;

bool shouldRefresh = true;

// TODO Use my Timer Class
unsigned long timerRadioRefresh = 0;
unsigned long timerLoop = 0;

void setup() {
  randomSeed(analogRead(0));
  
  out = new AudioOutputI2S(0, 1);
  out->SetOutputModeMono(true);
  out->SetGain(1.0);
  player = new AudioGeneratorMP3();

  M5.begin();
  
  M5.Lcd.setBrightness(100);
  M5.Lcd.setTextDatum(MC_DATUM);
  
  SD.begin();
  radio.begin();
  prefs.begin(PSTR("logger"));
  
  radioPowerOn = prefs.getBool(PSTR("radioPowerOn"), false);

  if (radioPowerOn) {
    checkRadio();
  }
  
  useSd = prefs.getBool(PSTR("useSd"), true);

  if (useSd) {
    checkSd();

    secondsBetweenPlaying = prefs.getUChar(PSTR("sBetweenPlay"), 7);
    playerMode = prefs.getUChar(PSTR("playerMode"), 0);
    if (playerMode >= nbMp3Directories) {
      playerMode = 0;
    } else {    
      autoMode = prefs.getBool(PSTR("autoMode"), false);
    }
  }
}

void loop() {
  M5.update();
  radio.flushRX();

  bool newIsTx = radioPowerOn && radio.isTx();
  shouldRefresh |= newIsTx != isTx;
  isTx = newIsTx;

  if (isTx && !player->isRunning()) {
    autoMode = false;
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
        checkSd();
        shouldRefresh = true;
        timerLoop = millis();
      } else if (M5.BtnB.pressedFor(1000, 1000)) {
        playAudioAndTx();
        shouldRefresh = true;
      } else if (M5.BtnC.pressedFor(1000, 1000)) {
        playerMode++;
        playerMode %= nbMp3Directories;
        shouldRefresh = true;
        timerLoop = millis();
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
    if (!player->isRunning() && !isTx && (millis() - timerRadioRefresh) >= TIME_BETWEEN_REFRESH_RADIO) {
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
        M5.Lcd.progressBar(20, 60, 220, 20, sMeter / (15 * 100));
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
      M5.Lcd.drawString("Playing", 160, 110);
    }

    if (useSd) {
      M5.Lcd.setTextSize(3);
      M5.Lcd.setTextColor(GREEN);
      M5.Lcd.drawString(mp3Directories[playerMode].name, 0, 220);

      M5.Lcd.setTextColor(CYAN);
      M5.Lcd.drawString((!autoMode ? "No auto " : "Auto ") + String(secondsBetweenPlaying) + "s", 0, 190);
    } else {
      M5.Lcd.setTextColor(RED);
      M5.Lcd.drawString("No SD", 0, 220);
    }
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
    radio.setMode(mode != CAT_MODE_FM ? CAT_MODE_DIG : CAT_MODE_PKT);
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
  
  prefs.putBool(PSTR("autoMode"), autoMode);
}

void playAudioAndTx() {
  prefs.putUChar(PSTR("playerMode"), playerMode);
  prefs.putUChar(PSTR("sBetweenPlay"), secondsBetweenPlaying);
  prefs.putBool(PSTR("autoMode"), autoMode);
  
  if (!player->isRunning() && !isTx) {    
    tx();
  
    file = new AudioFileSourceSD(getFileToPlay());
    id3 = new AudioFileSourceID3(file); 
    
    if (!player->begin(id3, out)) {
      stopAll();
      showErrorMessage("Audio KO");
    }
  }
}

bool checkRadio() {
  showStatusMessage("Radio check");
    
  radioPowerOn = false;

  radio.flushRX();
  unsigned long timer = millis();
  byte mode = radio.getMode();
  
  if (millis() - timer >= 1000 || isModeUnknown(mode)) {
    radioPowerOn = showErrorMessage(F("Radio issue"));
  } else {
    radioPowerOn = true;
  }
  
  prefs.putBool(PSTR("radioPowerOn"), radioPowerOn);
  
  return radioPowerOn;
}

bool checkSd() {
  useSd = false;
  
  showStatusMessage("SD check");
    
  File root = SD.open(PSTR("/"));
  if(!root || !root.isDirectory()){
    useSd = showErrorMessage(F("SD issue"));
  } else {
    File file = root.openNextFile();
    while(file && nbMp3Directories < MAX_MP3_DIRECTORIES) {
      if(file.isDirectory()) {
        struct Mp3 mp3Datas;
        mp3Datas.nbFiles = 0;
        mp3Datas.name[0] = '\0';
        strcpy(mp3Datas.name, file.name() + 1);
        
        File fileMp3 = file.openNextFile();
        while(fileMp3 && !fileMp3.isDirectory() && mp3Datas.nbFiles < MAX_MP3_IN_DIRECTORY) {
          mp3Datas.files[mp3Datas.nbFiles][0] = '\0';
          strcpy(mp3Datas.files[mp3Datas.nbFiles], fileMp3.name());
  
          mp3Datas.nbFiles++;
          
          fileMp3 = file.openNextFile();
        }
  
        if (mp3Datas.nbFiles > 0) {
          mp3Directories[nbMp3Directories++] = mp3Datas;
        }
      }
      file = root.openNextFile();
    }
    useSd = nbMp3Directories > 0;
  }
  
  prefs.putBool(PSTR("useSd"), useSd);
  
  return useSd;
}

bool showErrorMessage(String str) {
  M5.Lcd.clear();
  M5.Lcd.setTextColor(RED);
  M5.Lcd.setTextSize(3);
  M5.Lcd.drawString(str, 160, 100);
  delay(2000);

  return false;
}

void showStatusMessage(String str) {
  M5.Lcd.clear();
  M5.Lcd.setTextColor(ORANGE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.drawString(str, 160, 100);
}

const char* getFileToPlay() {
  return mp3Directories[playerMode].files[random(0, mp3Directories[playerMode].nbFiles + 1)];
}

bool isModeUnknown(byte mode) {
  switch (mode) {
    case CAT_MODE_LSB:
    case CAT_MODE_USB:
    case CAT_MODE_FM:
    case CAT_MODE_DIG:
    case CAT_MODE_PKT:
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

    case CAT_MODE_PKT:
      return "PKT";
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
