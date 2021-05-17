#ifndef HAMRADIO_M5STACK_LOGGER_PLAYER_H
#define HAMRADIO_M5STACK_LOGGER_PLAYER_H

#define USE_MP3

#ifdef USE_MP3
    #include <AudioGeneratorMP3.h>
    #include <AudioFileSourceID3.h>
#else
    #include <AudioGeneratorWAV.h>
#endif
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
    bool stopIfNeededOtherwisePlay(bool force);

    inline bool isActive() const {
        return active;
    }

    inline bool isPlaying() {
        return player.isRunning();
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

#ifdef USE_MP3
    // Use 8000 Hz sampling rate and 16 bits file encoding
    AudioGeneratorMP3 player;
#else
    AudioGeneratorWAV player;
#endif
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
