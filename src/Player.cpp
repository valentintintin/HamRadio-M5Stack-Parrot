#include "Player.h"

Player::Player(System* system): system(system), playerOut(AudioOutputI2S(0, 1)) {
    playerOut.SetOutputModeMono(true);
    playerOut.SetGain(1.0);
}

bool Player::init() {
    DPRINTLN(F("[Player] Init"));

    for (byte i = 0; i < nbCalls; i++) {
        delete calls[i];
    }
    nbCalls = 0;

    system->screen.showStatusMessage(PSTR("SD check"));

    File root = SD.open(PSTR("/"));
    if (!root || !root.isDirectory()) {
        system->screen.showErrorMessage(PSTR("SD issue"));
        active = false;
    } else {
        DPRINTLN(F("[Player] Search calls"));

        File fileRoot = root.openNextFile();
        while (fileRoot && nbCalls < MAX_CALLS) {
            if (fileRoot.isDirectory()) {
                DPRINT(F("[Player] Search calls in ")); DPRINTLN(fileRoot.name());

                Call* call = new Call(fileRoot.name());

                File soundFile = fileRoot.openNextFile();
                while (soundFile && !soundFile.isDirectory()) {
                    DPRINT(F("[Player] Add file ")); DPRINTLN(soundFile.name());

                    call->addFile(soundFile.name());

                    soundFile = fileRoot.openNextFile();
                }

                if (call->getNbFiles() > 0) {
                    calls[nbCalls++] = call;
                } else {
                    delete call;
                }
            }

            fileRoot = root.openNextFile();
        }

        DPRINT(F("[Player] Nb calls ")); DPRINTLN(nbCalls);
        for (byte iCall = 0; iCall < nbCalls; iCall++) {
            Call* call = calls[iCall];
            DPRINT(F("[Player] Call ")); DPRINT(call->getName()); DPRINT(F(" nbFiles ")); DPRINTLN(call->getNbFiles());
            for (byte iCallFile = 0; iCallFile < call->getNbFiles(); iCallFile++) {
                DPRINT(F("\tFile ")); DPRINTLN(call->getFile(iCallFile));
            }
        }

        active = nbCalls > 0;

        DPRINT(F("[Player] Active ")); DPRINTLN(active);

        setCurrentCallIndexUsed(system->prefs->getUChar(PSTR("callIndexUsed"), currentCallIndexUsed));
        setSecondsBetweenPlaying(system->prefs->getUChar(PSTR("sBetweenPlay"), secondsBetweenPlaying));
    }

    return isActive();
}

Call* Player::setCurrentCallIndexUsed(byte callIndex) {
    if (isActive()) {
        DPRINT(F("[Player] Want to set call index ")); DPRINTLN(callIndex);

        currentCallIndexUsed = callIndex % nbCalls;

        DPRINT(F("[Player] set call ")); DPRINT(callIndex); DPRINT(F(" ")); DPRINTLN(getCurrentCallUsed()->getName());

        system->prefs->putBool(PSTR("callIndexUsed"), currentCallIndexUsed);
        setAutoModeState(false);
    }

    return getCurrentCallUsed();
}

Call* Player::incrementCurrentCallIndexUsed() {
    setCurrentCallIndexUsed(currentCallIndexUsed + 1);
    return getCurrentCallUsed();
}

void Player::setAutoModeState(bool state) {
    if (isActive()) {
        DPRINT(F("[Player] set auto mode ")); DPRINTLN(state);

        autoMode = state;
        timerLoop.restart();
    }
}

void Player::setSecondsBetweenPlaying(byte seconds) {
    if (isActive()) {
        DPRINT(F("[Player] Want to set seconds auto mode ")); DPRINTLN(seconds);

        if (seconds > 15) {
            seconds = 5;
        }

        DPRINT(F("[Player] set seconds auto mode ")); DPRINTLN(seconds);

        secondsBetweenPlaying = seconds;
        system->prefs->putUChar(PSTR("sBetweenPlay"), secondsBetweenPlaying);
        timerLoop.setInterval(secondsBetweenPlaying * 1000);
        setAutoModeState(false);
    }
}

byte Player::incrementSecondsBetweenPlaying(byte increment) {
    setSecondsBetweenPlaying(secondsBetweenPlaying + increment);
    return getSecondsBetweenPlaying();
}

bool Player::play() {
    DPRINTLN(F("[Player] Play"));

    if (!player.isRunning()) {
        Call* callUsed = getCurrentCallUsed();
        if (callUsed == nullptr) {
            system->screen.showErrorMessage(PSTR("Call KO"));
            DPRINTLN(F("[Player] Play KO no call"));
            return false;
        }

        fileToPlay[0] = '\0';
        strcpy(fileToPlay, callUsed->getFileToPlay());

        DPRINT(F("[Player] Play ")); DPRINTLN(fileToPlay);

        playerFile = new AudioFileSourceSD(fileToPlay);

        if (!player.begin(playerFile, &playerOut)) {
            stopIfNeededOtherwisePlay(true);
            system->screen.showErrorMessage(PSTR("Audio KO"));
            DPRINTLN(F("[Player] Play KO"));
            return false;
        }

        DPRINTLN(F("[Player] Play OK"));

        return true;
    }

    DPRINTLN(F("[Player] Play KO already running"));

    return false;
}

bool Player::stopIfNeededOtherwisePlay(bool force) {
    if (!player.loop() || force) {
        DPRINTLN(F("[Player] Stop"));

        player.stop();

        delete playerFile;

        timerLoop.restart();

        return true;
    }

    return false;
}

byte Player::getSecondsBeforePlaying() const {
    if (isAutoModeEnabled()) {
        return timerLoop.getTimeLeft() / 1000;
    }

    return 0;
}