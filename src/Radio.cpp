#include "Radio.h"

Radio::Radio(System* system) : system(system), timerRefresh(Timer(TIME_BETWEEN_REFRESH_RADIO)) {
    radio.begin();
}

bool Radio::init() {
    DPRINTLN(F("[Radio] Init"));

    return checkRadio();
}

bool Radio::checkRadio() {
    system->screen.showStatusMessage(PSTR("Radio check"));

    active = false;

    radio.flushRX();

    unsigned long timeTimeout = millis();
    currentMode = radio.getMode();

    if (millis() - timeTimeout >= TIME_TIMEOUT_RADIO || isModeUnknown(currentMode)) {
        system->screen.showErrorMessage(PSTR("Radio KO"));
        active = false;
        currentMode = 255;
    } else {
        active = true;
    }

    system->prefs->putBool(PSTR("active"), active);

    DPRINT(F("[Radio] Check ")); DPRINTLN(active);

    return active;
}

bool Radio::update() {
    changedDetected = NOTHING;

    if (active) {
        radio.flushRX();

        bool newTxState = radio.isTx();
        if (newTxState != txState) changedDetected |= CHANGE_TX_STATE;
        txState = newTxState;

        if (isRx() && timerRefresh.hasExpired()) {
            unsigned long newFreq = (unsigned int) (radio.getFrequency() / 100);
            if (newFreq != currentFreq) changedDetected |= CHANGE_FREQ;
            currentFreq = newFreq;

            byte newMode = radio.getMode();
            if (newMode != currentMode) changedDetected |= CHANGE_MODE;
            currentMode = newMode;

            byte newSMeter = radio.getSMeter();
            if (newSMeter != currentSMeter) changedDetected |= CHANGE_S_METER;
            currentSMeter = newSMeter;

            timerRefresh.restart();
        }

        if (hasChanged()) {
            DPRINT(F("[Radio] Change detected : ")); DPRINTLN(changedDetected);
            DPRINT(F("\tPTT (TX) ")); DPRINTLN(txState);
            DPRINT(F("\tFreq ")); DPRINTLN(currentFreq);
            DPRINT(F("\tMode ")); DPRINTLN(currentMode);
            DPRINT(F("\tS-Meter ")); DPRINTLN(currentSMeter);
        }
    }

    return hasChanged();
}

bool Radio::tx() {
    if (active) {
        DPRINTLN(F("[Radio] Set PPT On"));

        radio.setMode(currentMode != CAT_MODE_FM ? CAT_MODE_DIG : CAT_MODE_PKT);
        radio.setPtt(true);
        radio.flushRX();
        txState = radio.isTx();
        if (!txState) {
            rx();

            DPRINTLN(F("[Radio] Set PPT On KO"));

            return false;
        }
        DPRINTLN(F("[Radio] Set PPT On OK"));

        delay(300);

        return true;
    }

    return false;
}

bool Radio::rx() {
    if (active) {
        DPRINTLN(F("[Radio] Set PPT Off"));

        delay(300);
        radio.flushRX();
        radio.setPtt(false);
        radio.flushRX();
        delay(300);
        radio.flushRX();
        radio.setMode(currentMode);
        txState = radio.isTx();

        if (!isRx()) {
            DPRINTLN(F("[Radio] Set PPT Off KO"));
            return false;
        }

        DPRINTLN(F("[Radio] Set PPT Off OK"));
        return true;
    }

    return false;
}

bool Radio::isModeUnknown(byte mode) const {
    switch (mode) {
        case CAT_MODE_LSB:
        case CAT_MODE_USB:
        case CAT_MODE_FM:
        case CAT_MODE_DIG:
        case CAT_MODE_PKT:
            return false;

        default:
            return true;
    }
}

const char* Radio::getStringMode(byte mode) const {
    switch (mode) {
        case CAT_MODE_LSB:
            return PSTR("LSB");

        case CAT_MODE_USB:
            return PSTR("USB");

        case CAT_MODE_DIG:
            return PSTR("DIG");

        case CAT_MODE_FM:
            return PSTR("FM");

        case CAT_MODE_PKT:
            return PSTR("PKT");

        default:
            return PSTR("UNK");
    }
}

const char* Radio::getStringSMeter(byte sMeterValue) const {
    switch (sMeterValue) {
        case 0:
            return PSTR("S0");

        case 1:
            return PSTR("S1");

        case 2:
            return PSTR("S2");

        case 3:
            return PSTR("S3");

        case 4:
            return PSTR("S4");

        case 5:
            return PSTR("S5");

        case 6:
            return PSTR("S6");

        case 7:
            return PSTR("S7");

        case 8:
            return PSTR("S8");

        case 9:
            return PSTR("S9");

        case 10:
            return PSTR("S9+10");

        case 11:
            return PSTR("S9+20");

        case 12:
            return PSTR("S9+30");

        case 13:
            return PSTR("S9+40");

        case 14:
            return PSTR("S9+50");

        case 15:
            return PSTR("S9+60");

        default:
            return PSTR("S-UNK");
    }
}