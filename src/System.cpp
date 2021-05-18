#include <M5Stack.h>
#include <Call.h>

#include "System.h"

System::System(Preferences *prefs): prefs(prefs), radio(new Radio(this)), player(new Player(this)) {
}

void System::init() {
    M5.begin();

    DPRINTLN(F("[System] Start"));

    randomSeed(analogRead(0));

    prefs->begin(PSTR("logger"));

    DPRINTLN(F("[System] Start OK"));

    check();
}

void System::check() {
    DPRINTLN(F("[System] Check"));

    screen.init();
    radio->init();
    player->init();

    DPRINTLN(F("[System] Check end"));
}

void System::update() {
    M5.update();
    shouldRefreshScreen |= radio->update();

    if (shouldRefreshScreen && !player->isPlaying()) { // Change on radio and not playing
        DPRINTLN(F("[System] Radio TX & Player stopped"));
        player->setAutoModeState(false);
    }

    if (!player->isPlaying() && !radio->isTx()) { // Radio RX and audio stopped
        if (player->hasAutoModeTimerExpired()) {
            DPRINTLN(F("[System] Player auto mode expired"));
            playAudioAndTx();
            shouldRefreshScreen = true;
        } else {
            if (M5.BtnB.wasReleased()) {
                DPRINTLN(F("[System] Button B"));
                player->setAutoModeState(!player->isAutoModeEnabled());
                shouldRefreshScreen = true;
            } else if (M5.BtnC.wasReleased()) {
                DPRINTLN(F("[System] Button C"));
                player->incrementSecondsBetweenPlaying();
                shouldRefreshScreen = true;
            } else if (M5.BtnA.pressedFor(1000, 1000)) {
                DPRINTLN(F("[System] Button A long"));
                check();
                shouldRefreshScreen = true;
            } else if (M5.BtnB.pressedFor(1000, 1000)) {
                DPRINTLN(F("[System] Button B long"));
                playAudioAndTx();
                shouldRefreshScreen = true;
            } else if (M5.BtnC.pressedFor(1000, 1000)) {
                DPRINTLN(F("[System] Button C long"));
                player->incrementCurrentCallIndexUsed();
                shouldRefreshScreen = true;
            }

            if (player->isAutoModeEnabled()) {
                byte newSecondsBeforePlaying = player->getSecondsBeforePlaying();
                if (newSecondsBeforePlaying != lastSecondsBeforePlaying) {
                    shouldRefreshScreen = true;
                    DPRINT(F("[System] Seconds before playing ")); DPRINTLN(newSecondsBeforePlaying);
                }
                lastSecondsBeforePlaying = newSecondsBeforePlaying;
            }
        }
    }

    if (M5.BtnA.wasPressed()) {
        DPRINTLN(F("[System] Button A"));
        stopAll();
        shouldRefreshScreen = true;
    }

    if (stopPlayerAndRxIfNeeded(false)) {
        shouldRefreshScreen = true;
    }

    if (radio->isActive()) {
        if (shouldRefreshScreen) {
            M5.Lcd.clear();
            M5.Lcd.setTextColor(WHITE);
            M5.Lcd.setTextSize(3);

            radio->getCurrentStringFreq(bufferScreen);
            strcat_P(bufferScreen, PSTR(" "));
            strcat(bufferScreen, radio->getCurrentStringMode());
            M5.Lcd.drawString(bufferScreen, 160, 0);

            if (radio->isTx()) {
                M5.Lcd.setTextColor(RED);
                M5.Lcd.setTextSize(5);
                M5.Lcd.drawString(PSTR("TX"), 160, 70);
            } else {
                M5.Lcd.drawString(radio->getCurrentStringSMeter(), 160, 40);
                M5.Lcd.progressBar(20, 60, 280, 20, radio->getCurrentSMeter() * (100 / 15));
            }
        }
    } else if (shouldRefreshScreen) {
        M5.Lcd.clear();
        M5.Lcd.setTextColor(RED);
        M5.Lcd.setTextSize(3);
        M5.Lcd.drawString(PSTR("No radio"), 160, 0);
    }

    if (shouldRefreshScreen) {
        Call* call = player->getCurrentCallUsed();

        if (player->isActive() && call != nullptr) {
            if (player->isPlaying()) {
                M5.Lcd.setTextColor(GREEN);
                M5.Lcd.setTextSize(5);
                M5.Lcd.drawString(PSTR("Playing"), 160, 110);
            }

            M5.Lcd.setTextSize(3);
            M5.Lcd.setTextColor(GREEN);
            M5.Lcd.drawString(call->getName(), 0, 220);

            M5.Lcd.setTextColor(CYAN);
            if (player->isAutoModeEnabled()) {
                sprintf_P(bufferScreen, PSTR("Auto %ds"), lastSecondsBeforePlaying);
            } else {
                sprintf_P(bufferScreen, PSTR("No auto %ds"), player->getSecondsBetweenPlaying());
            }

            M5.Lcd.drawString(bufferScreen, 0, 190);
        } else {
            M5.Lcd.setTextColor(RED);
            M5.Lcd.drawString(PSTR("No SD"), 0, 220);
        }
    }

    shouldRefreshScreen = false;
}

bool System::stopPlayerAndRxIfNeeded(bool force) {
    if (player->isPlaying()) {
        if (player->updateOrStopForce(force)) {
            radio->rx();

            return true;
        }
    }

    if (force) {
        radio->rx();

        return true;
    }

    return false;
}

bool System::stopAll() {
    DPRINTLN(F("[System] Stop all"));

    player->setAutoModeState(false);
    return stopPlayerAndRxIfNeeded(true);
}

bool System::playAudioAndTx() {
    DPRINTLN(F("[System] Play & TX"));

    if (radio->isTx()) {
        DPRINTLN(F("[System] Play & TX KO already TX"));
        return false;
    }

    if (radio->isActive() && !radio->tx()) {
        DPRINTLN(F("[System] Play & TX KO can't TX"));
        return false;
    }

    if (!player->play()) {
        stopAll();

        DPRINTLN(F("[System] Play & TX KO"));
        return false;
    }

    DPRINTLN(F("[System] Play & TX OK"));
    return true;
}