#include <M5Stack.h>

#include "System.h"
#include "Screen.h"

void Screen::init() {
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setBrightness(100);
    M5.Lcd.setRotation(3); // 180° (buttons top)

    showStatusMessage(PSTR("Init..."));
}

void Screen::showErrorMessage(const String &str) const {
    DPRINT(F("[Screen] Error ")); DPRINTLN(str);

    M5.Lcd.clear();
    M5.Lcd.setTextColor(RED);
    M5.Lcd.setTextSize(3);
    M5.Lcd.drawString(str, 160, 100);
    delay(2000);
}

void Screen::showStatusMessage(const String &str) const {
    DPRINT(F("[Screen] Status ")); DPRINTLN(str);

    M5.Lcd.clear();
    M5.Lcd.setTextColor(ORANGE);
    M5.Lcd.setTextSize(3);
    M5.Lcd.drawString(str, 160, 100);
}
