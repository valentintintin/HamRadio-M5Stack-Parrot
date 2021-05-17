#ifndef HAMRADIO_M5STACK_LOGGER_SCREEN_H
#define HAMRADIO_M5STACK_LOGGER_SCREEN_H

// https://github.com/m5stack/m5-docs/blob/master/docs/en/api/lcd.md

class Screen {
public:
    void init();

    void showErrorMessage(const String &str) const;
    void showStatusMessage(const String &str) const;

private:
    bool shouldRefresh = true;
};


#endif //HAMRADIO_M5STACK_LOGGER_SCREEN_H
