#include "Call.h"

Call::Call(const char* callName) {
    name[0] = '\0';
    strcpy(name, callName + 1);
}

void Call::addFile(const char *fileName) {
    if (nbFiles < MAX_FILES_FOR_CALL) {
        files[nbFiles][0] = '\0';
        strcpy(files[nbFiles], fileName);

        nbFiles++;
    }
}

char* Call::getFileToPlay() {
    return files[random(0, nbFiles)];
}