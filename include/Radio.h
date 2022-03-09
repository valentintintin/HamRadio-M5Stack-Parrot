#ifndef HAMRADIO_M5STACK_LOGGER_RADIO_H
#define HAMRADIO_M5STACK_LOGGER_RADIO_H

#include <FT857.h>

#include "System.h"
#include "../lib/Timer/Timer.h"

#define TIME_BETWEEN_REFRESH_RADIO 350
#define TIME_TIMEOUT_RADIO 1000

#define CHANGE_ALL 0xFF
#define NOTHING 0
#define CHANGE_FREQ 0b1
#define CHANGE_MODE 0b10
#define CHANGE_S_METER 0b100
#define CHANGE_TX_STATE 0b1000

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

    inline bool hasChanged(byte mode = CHANGE_ALL) const {
        return (changedDetected & mode) > 0;
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

    inline const char* getCurrentStringMode() const {
        return getStringMode(getCurrentMode());
    }

    inline byte getCurrentSMeter() const {
        return currentSMeter;
    }

    inline const char* getCurrentStringSMeter() const {
        return getStringSMeter(getCurrentSMeter());
    }

    inline unsigned long getCurrentFreq() const {
        return currentFreq;
    }

    inline void getCurrentStringFreq(char *dest) const {
        dtostrf(getCurrentFreq() / 1000.0, (3 + 2), 3, dest);
        strcat_P(dest, PSTR(" Mhz"));
    }

private:
    System* system;

    bool active = false;
    byte changedDetected = 0;

    unsigned long currentFreq = 0;
    byte currentMode = 255;
    byte currentSMeter = 255;
    bool txState = false;

    Timer timerRefresh;

    FT857 radio;

    bool isModeUnknown(byte mode) const ;
    const char* getStringMode(byte mode) const;
    const char * getStringSMeter(byte sMeterValue) const;
};


#endif //HAMRADIO_M5STACK_LOGGER_RADIO_H
