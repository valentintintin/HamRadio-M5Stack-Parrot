#ifndef HAMRADIO_M5STACK_LOGGER_CALL_H
#define HAMRADIO_M5STACK_LOGGER_CALL_H

#include <Arduino.h>

#define MAX_CALL_NAME 16
#define MAX_FILES_FOR_CALL 3
#define MAX_FILE_NAME 255
#define MAX_FILE_NAME_TO_PLAY MAX_FILE_NAME + MAX_CALL_NAME + 4 // 4 = / + one more / and safety

struct AudioFile {
    char filename[MAX_FILE_NAME_TO_PLAY];
    bool isMp3;
};

class Call {
public:
    explicit Call(const char* callName);

    void addFile(const char* fileName);
    struct AudioFile* getFileToPlay();

    inline byte getNbFiles() const {
        return nbFiles;
    }

    inline struct AudioFile* getFile(int index) {
        return &files[index];
    }

    inline char* getName() {
        return name;
    }
private:
    char name[MAX_CALL_NAME];
    struct AudioFile files[MAX_FILES_FOR_CALL];
    byte nbFiles = 0;
};


#endif //HAMRADIO_M5STACK_LOGGER_CALL_H
