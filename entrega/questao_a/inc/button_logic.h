#ifndef APP_BUTTON_LOGIC_H
#define APP_BUTTON_LOGIC_H

#include <stdbool.h>
#include <stdint.h>

#define BUTTON_DEBOUNCE_MS 30U
#define BUTTON_QUEUE_CAPACITY 32U

typedef struct {
    uint32_t number;
    uint32_t timestamp_ms;
} button_event_t;

/* Logica sem dependencia de hardware. O adaptador button.c protege os acessos. */
typedef struct {
    bool raw_pressed;
    bool pending;
    bool armed;
    uint32_t since_ms;
    uint32_t press_count;
    uint32_t lost_total;
    uint32_t head;
    uint32_t count;
    button_event_t events[BUTTON_QUEUE_CAPACITY];
} button_logic_t;

void button_logic_init(button_logic_t *state, bool pressed, uint32_t now);
void button_logic_edge(button_logic_t *state, bool pressed, uint32_t now);
void button_logic_tick(button_logic_t *state, uint32_t now);
bool button_logic_peek(const button_logic_t *state, button_event_t *event);
void button_logic_pop(button_logic_t *state);

#endif
