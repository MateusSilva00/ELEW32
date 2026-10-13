#pragma once

#include <stdint.h>

void serial_init(uint32_t baud);
void serial_write(const char *text);
char serial_read_char(void);
