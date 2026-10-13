#ifndef APP_TIMEBASE_H
#define APP_TIMEBASE_H

#include <stdbool.h>
#include <stdint.h>

/* Chamar uma vez, depois de configurar SystemCoreClock. */
bool timebase_init(void);
uint32_t millis(void);

#endif
