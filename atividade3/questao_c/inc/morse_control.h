#ifndef APP_MORSE_CONTROL_H
#define APP_MORSE_CONTROL_H
#include "morse_logic.h"
#define MORSE_DEBOUNCE_MS 30U
#define MORSE_QUEUE_CAPACITY 16U
typedef struct {
    morse_logic_t logic;
    bool active;
    bool ready;
    bool raw_pressed;
    bool stable_pressed;
    bool pending;
    uint32_t edge_at;
    morse_result_t events[MORSE_QUEUE_CAPACITY];
    uint32_t head;
    uint32_t count;
    uint32_t lost_total;
} morse_control_t;
void morse_control_init(morse_control_t *state, bool active, bool pressed, uint32_t now);
void morse_control_edge(morse_control_t *state, bool pressed, uint32_t now);
void morse_control_tick(morse_control_t *state, uint32_t now);
bool morse_control_peek(const morse_control_t *state, morse_result_t *result);
void morse_control_pop(morse_control_t *state);
#endif
