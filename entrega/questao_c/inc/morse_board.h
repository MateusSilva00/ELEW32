#ifndef APP_MORSE_BOARD_H
#define APP_MORSE_BOARD_H
#include "morse_logic.h"
void morse_board_init(void);
void morse_board_tick(uint32_t now);
/* APIs do main: protegem estado compartilhado com EXTI/SysTick. */
void morse_board_session(bool active);
bool morse_board_peek(morse_result_t *result);
void morse_board_pop(void);
uint32_t morse_board_lost(void);
#endif
