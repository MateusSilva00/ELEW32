#ifndef APP_REACTION_CONTROL_H
#define APP_REACTION_CONTROL_H
#include "reaction_logic.h"
#define REACTION_DEBOUNCE_MS 30U
typedef struct {
    reaction_logic_t logic;
    bool raw_pressed;
    bool pending;
    bool armed;
    uint32_t edge_at;
} reaction_control_t;
void reaction_control_init(reaction_control_t *state, bool pressed, uint32_t now);
void reaction_control_edge(reaction_control_t *state, bool pressed, uint32_t now);
void reaction_control_tick(reaction_control_t *state, uint32_t now);
bool reaction_control_released(const reaction_control_t *state);
bool reaction_control_start(reaction_control_t *state, uint32_t now);
bool reaction_control_stimulus(reaction_control_t *state, uint32_t now);
#endif
