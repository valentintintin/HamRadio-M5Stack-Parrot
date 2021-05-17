#include "Radio.h"

Radio::Radio(System* system) : system(system), timerRefresh(Timer(TIME_BETWEEN_REFRESH_RADIO)) {
}

bool Radio::init() {
    DPRINTLN(F("[Radio] Init"));

    radio.begin();

    if (system->prefs->getBool(PSTR("active"), false)) {
        return checkRadio();
    }

    DPRINTLN(F("[Radio] Init KO"));

    return false;
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
    changedDetected = false;

    if (active) {
        DPRINTLN(F("[Radio] Update"));

        radio.flushRX();

        DPRINT(F("[Radio] Get TX state "));
        bool newTxState = radio.isTx();

        if (newTxState != txState) {
            changedDetected = true;
        }
        txState = newTxState;
        DPRINTLN(txState);

        if (isRx() && timerRefresh.hasExpired()) {
            DPRINT(F("[Radio] Get freq"));
            unsigned long newFreq = (unsigned int) (radio.getFrequency() / 100);
            changedDetected |= newFreq != currentFreq;
            currentFreq = newFreq;
            DPRINTLN(currentFreq);

            DPRINT(F("[Radio] Get mode"));
            byte newMode = radio.getMode();
            changedDetected |= newMode != currentMode;
            currentMode = newMode;
            DPRINTLN(currentMode);

            DPRINT(F("[Radio] Get s-meter"));
            byte newSMeter = radio.getSMeter();
            changedDetected |= newSMeter != currentSMeter;
            currentSMeter = newSMeter;
            DPRINTLN(currentSMeter);

            timerRefresh.restart();
        }

        DPRINT(F("[Radio] Change detected ")); DPRINTLN(hasChanged());
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

String Radio::getStringMode(byte mode) const {
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

        default:
            return "UNK";
    }
}

String Radio::getStringSMeter(byte sMeterValue) const {
    if (sMeterValue < 10) {
        return "S" + String(sMeterValue);
    }
    else {
        switch (sMeterValue) {
            case 10:
                return "S9+10";

            case 11:
                return "S9+20";

            case 12:
                return "S9+30";

            case 13:
                return "S9+40";

            case 14:
                return "S9+50";

            case 15:
                return "S9+60";

            default:
                return "S-UNK";
        }
    }
}