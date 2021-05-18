#ifndef HAMRADIO_M5STACK_LOGGER_SYSTEM_H
#define HAMRADIO_M5STACK_LOGGER_SYSTEM_H


#define DEBUG

#ifdef DEBUG
#define DPRINT(...) Serial.print(__VA_ARGS__)
#define DPRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define DPRINT(...)
#define DPRINTLN(...)
#endif

#include <Preferences.h>

#include "Screen.h"
#include "Radio.h"
#include "Player.h"

class Radio;
class Player;

class System {
public:
    explicit System(Preferences *prefs);

    void init();
    void check();
    void update();

    bool stopPlayerAndRxIfNeeded(bool force);
    bool stopAll();
    bool playAudioAndTx();

    Preferences* prefs;
    Screen screen;
private:
    char bufferScreen[255];
    Radio* radio;
    Player* player;

    bool shouldRefreshScreen = true;
    byte lastSecondsBeforePlaying = 255;
};


#endif //HAMRADIO_M5STACK_LOGGER_SYSTEM_H
