#pragma once

#include <unistd.h>

void scramble_process_name(unsigned int seed);
void stomp_flag_loop(char *file, char *content);
void stomp_flag(char *file, char *content, size_t len);
void stomp_flag_ro(char *file, char *content, size_t len);
