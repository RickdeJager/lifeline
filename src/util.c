#define _GNU_SOURCE
#include <pthread.h>   // ..
#include <stdlib.h>    // rand
#include <string.h>    // strlen
#include <sys/param.h> // Change process name
#include <sys/prctl.h> // ..

#include "constants.h"

void scramble_process_name(int argc, char *argv[]) {
    // Pick a random process name
    unsigned int upper = sizeof(process_names) / sizeof(process_names[0]);
    unsigned int r = rand() % upper;
    const char *name = process_names[r];

    // Change our process name
    // https://stackoverflow.com/questions/6082189/change-process-name-in-linux
    pthread_setname_np(pthread_self(), name);
    prctl(PR_SET_NAME, (unsigned long)name, 0, 0, 0);

    // Change the Commandline seen in ps
    // --> [Process Name, NULL, NULL, ...]
    for (unsigned int i = 0; i < argc; i++) {
        unsigned long length = MAX(strlen(argv[i]), strlen(name));
        for (unsigned int j = 0; j < length; j++) {
            if (i == 0 && j < strlen(name)) {
                argv[i][j] = name[j];
            } else {
                argv[i][j] = 0x00;
            }
        }
    }
}
