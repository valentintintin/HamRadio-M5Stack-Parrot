#ifndef HAMRADIO_M5STACK_LOGGER_CALL_H
#define HAMRADIO_M5STACK_LOGGER_CALL_H

#include <Arduino.h>

#define MAX_CALL_NAME 16
#define MAX_FILES_FOR_CALL 3
#define MAX_FILE_NAME 255
#define MAX_FILE_NAME_TO_PLAY MAX_FILE_NAME + MAX_CALL_NAME + 4 // 4 = / + one more / and safety

class Call {
public:
    explicit Call(const char* callName);

    void addFile(const char* fileName);
    char* getFileToPlay();

    inline byte getNbFiles() const {
        return nbFiles;
    }

    inline char* getFile(int index) {
        return files[index];
    }

    inline char* getName() {
        return name;
    }
private:
    char name[MAX_CALL_NAME];
    char files[MAX_FILES_FOR_CALL][MAX_FILE_NAME_TO_PLAY];
    byte nbFiles = 0;
};


#endif //HAMRADIO_M5STACK_LOGGER_CALL_H
