#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>

#include "System.h"

Preferences preferences;
System systemEngine(&preferences);

void setup() {
    WiFi.mode(WIFI_OFF);

    systemEngine.init();
}

void loop() {
    systemEngine.update();

    delay(10);
}