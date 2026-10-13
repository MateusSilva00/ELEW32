#ifndef APP_RHYTHM_BOARD_H
#define APP_RHYTHM_BOARD_H
#include "rhythm_logic.h"
typedef struct {
    uint8_t steps;
    rhythm_class_t pattern[RHYTHM_MAX_STEPS];
} rhythm_sequence_t;
void rhythm_board_init(void);
void rhythm_board_tick(uint32_t now);
/* APIs do main: protegem estado compartilhado com EXTI/SysTick. */
void rhythm_board_session(bool active);
rhythm_sequence_t rhythm_board_sequence(void);
bool rhythm_board_start(void);
bool rhythm_board_next(void);
bool rhythm_board_peek(rhythm_event_t *event);
void rhythm_board_pop(void);
#endif
