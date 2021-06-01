#ifndef HAMRADIO_M5STACK_LOGGER_PLAYER_H
#define HAMRADIO_M5STACK_LOGGER_PLAYER_H

#include <AudioGeneratorMP3.h>
#include <AudioFileSourceID3.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>
#include <AudioFileSourceSD.h>

#include "System.h"
#include "../lib/Timer/Timer.h"
#include "Call.h"

#define MAX_CALLS 10

class System;

class Player {
public:
    explicit Player(System* system);

    bool init();

    Call* setCurrentCallIndexUsed(byte callIndex);
    Call* incrementCurrentCallIndexUsed();
    void setAutoModeState(bool state);
    void setSecondsBetweenPlaying(byte seconds);
    byte getSecondsBeforePlaying() const;
    byte incrementSecondsBetweenPlaying(byte increment = 1);
    bool play();
    bool updateOrStopForce(bool forceStop);

    inline bool isActive() const {
        return active;
    }

    inline bool isPlaying() {
        return currentPlayer->isRunning();
    }

    inline Call* getCurrentCallUsed() const {
        if (active && currentCallIndexUsed < nbCalls) {
            return calls[currentCallIndexUsed];
        }

        return nullptr;
    }

    inline bool isAutoModeEnabled() const {
        return isActive() && autoMode;
    }

    inline bool hasAutoModeTimerExpired() const {
        return isAutoModeEnabled() && timerLoop.hasExpired();
    }

    inline byte getSecondsBetweenPlaying() const {
        return secondsBetweenPlaying;
    }

private:
    System* system;

    // Use 8000 Hz sampling rate and 16 bits file encoding
    AudioGeneratorMP3 playerMp3;
    AudioGeneratorWAV playerWav;
    AudioGenerator *currentPlayer = &playerMp3;
    AudioOutputI2S playerOut;
    AudioFileSourceSD *playerFile;
    char fileToPlay[MAX_FILE_NAME_TO_PLAY];

    bool active = false;
    byte currentCallIndexUsed = 0;
    bool autoMode = false;
    byte secondsBetweenPlaying = 5;

    Call* calls[MAX_CALLS];
    byte nbCalls = 0;

    Timer timerLoop;
};


#endif //HAMRADIO_M5STACK_LOGGER_PLAYER_H
