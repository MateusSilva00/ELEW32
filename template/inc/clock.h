#ifndef APP_CLOCK_H
#define APP_CLOCK_H

#include <stdbool.h>

/* Chamar uma vez, apos reset e antes de iniciar perifericos/SysTick. */
bool clock_init_96mhz(void);

#endif
