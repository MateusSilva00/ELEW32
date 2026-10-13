#ifndef APP_RHYTHM_CONTROL_H
#define APP_RHYTHM_CONTROL_H
#include "rhythm_logic.h"
#define RHYTHM_DEBOUNCE_MS 30U
/* Uma rodada gera no maximo RHYTHM_MAX_STEPS eventos, e a proxima so comeca
 * depois que o main consome o evento final. A fila nunca transborda. */
#define RHYTHM_QUEUE_CAPACITY RHYTHM_MAX_STEPS
typedef struct {
    rhythm_logic_t logic;
    bool active;
    bool ready;
    bool raw_pressed;
    bool stable_pressed;
    bool pending;
    uint32_t edge_at;
    rhythm_event_t events[RHYTHM_QUEUE_CAPACITY];
    uint32_t head;
    uint32_t count;
} rhythm_control_t;
void rhythm_control_init(rhythm_control_t *state, bool active, bool pressed,
                         uint32_t now, uint32_t seed);
void rhythm_control_edge(rhythm_control_t *state, bool pressed, uint32_t now);
void rhythm_control_tick(rhythm_control_t *state, uint32_t now);
bool rhythm_control_led(const rhythm_control_t *state, uint32_t now);
bool rhythm_control_start(rhythm_control_t *state, uint32_t now);
bool rhythm_control_peek(const rhythm_control_t *state, rhythm_event_t *event);
void rhythm_control_pop(rhythm_control_t *state);
#endif
