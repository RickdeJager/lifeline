#define _GNU_SOURCE
#include <pthread.h>   // ..
#include <stdlib.h>    // rand
#include <string.h>    // strlen
#include <sys/param.h> // Change process name
#include <sys/prctl.h> // ..

#include "constants.h"

void padded_strcpy(char *dst, char *src) {
    unsigned long src_len = strlen(src);
    unsigned long dst_len = strlen(dst);
    unsigned long length = MAX(src_len, dst_len) + 1;
    for (unsigned int i = 0; i < length; i++) {
        if (i < src_len) {
            dst[i] = src[i];
        } else {
            dst[i] = 0x00;
        }
    }
}

void scramble_process_name(int argc, char *argv[]) {
    // Pick a random process name
    unsigned int upper = sizeof(process_names) / sizeof(process_names[0]);
    unsigned int r = rand() % upper;
    const char *name = process_names[r];

    // Change our process name
    // https://stackoverflow.com/questions/6082189/change-process-name-in-linux
    pthread_setname_np(pthread_self(), name);
    prctl(PR_SET_NAME, (unsigned long)name, 0, 0, 0);

    // Change the Commandline seen in ps by directly reassigning argv
    padded_strcpy(argv[0], (char *)name);
    for (int i = 1; i < argc; i++)
        padded_strcpy(argv[i], "");

    // set the argv[0] pointer, which will cause some programs (like htop iirc) to read
    // the program name from the new location.
    argv[0] = (char *)name;
}
