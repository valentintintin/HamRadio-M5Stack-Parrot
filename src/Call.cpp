#include "Call.h"
#include <Arduino.h>

Call::Call(const char* callName) {
    name[0] = '\0';
    strcpy(name, callName + 1);
}

void Call::addFile(const char *fileName) {
    if (nbFiles < MAX_FILES_FOR_CALL) {
        struct AudioFile file{};

        file.isMp3 = strstr_P(fileName, PSTR(".mp3")) != nullptr;
        file.filename[0] = '\0';
        strcpy(file.filename, fileName);

        files[nbFiles] = file;
        nbFiles++;
    }
}

struct AudioFile* Call::getFileToPlay() {
    return getFile(random(0, nbFiles));
}
