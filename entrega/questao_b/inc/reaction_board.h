#ifndef APP_REACTION_BOARD_H
#define APP_REACTION_BOARD_H
#include "reaction_logic.h"
typedef struct {
    reaction_phase_t phase;
    uint32_t reaction_ms;
} reaction_status_t;
void reaction_board_init(void);
void reaction_board_tick(uint32_t now);
/* APIs abaixo usadas somente pelo main; protegem estado compartilhado. */
void reaction_board_new_session(void);
void reaction_board_reset(void);
bool reaction_board_start(void);
reaction_status_t reaction_board_status(void);
#endif
