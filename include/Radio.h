#ifndef HAMRADIO_M5STACK_LOGGER_RADIO_H
#define HAMRADIO_M5STACK_LOGGER_RADIO_H

#include <FT857.h>

#include "System.h"
#include "../lib/Timer/Timer.h"

#define TIME_BETWEEN_REFRESH_RADIO 350
#define TIME_TIMEOUT_RADIO 1000

class System;

class Radio {
public:
    Radio(System* system);

    bool init();
    bool checkRadio();

    bool update();

    bool tx();
    bool rx();

    inline bool isActive() const {
        return active;
    }

    inline bool hasChanged() const {
        return changedDetected;
    }

    inline bool isTx() const {
        return txState;
    }

    inline bool isRx() const {
        return !isTx();
    }

    inline byte getCurrentMode() const {
        return currentMode;
    }

    inline String getCurrentStringMode() const {
        return getStringMode(getCurrentMode());
    }

    inline byte getCurrentSMeter() const {
        return currentSMeter;
    }

    inline String getCurrentStringSMeter() const {
        return getStringSMeter(getCurrentSMeter());
    }

    inline unsigned long getCurrentFreq() const {
        return currentFreq;
    }

    inline String getCurrentStringFreq() const {
        return String(getCurrentFreq() / 1000.0, 3) + " MHz ";
    }

private:
    System* system;

    bool active = false;
    bool changedDetected = false;

    unsigned long currentFreq = 0;
    byte currentMode = 255;
    byte currentSMeter = 255;
    bool txState = false;

    Timer timerRefresh;

    FT857 radio;

    bool isModeUnknown(byte mode) const ;
    String getStringMode(byte mode) const;
    String getStringSMeter(byte sMeterValue) const;
};


#endif //HAMRADIO_M5STACK_LOGGER_RADIO_H
