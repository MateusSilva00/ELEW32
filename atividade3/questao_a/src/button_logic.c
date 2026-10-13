#include "button_logic.h"

void button_logic_init(button_logic_t *state, bool pressed, uint32_t now)
{
    *state = (button_logic_t){0};
    /* Exige soltura estavel inclusive se ligar a placa com o botao apertado. */
    button_logic_edge(state, pressed, now);
}

void button_logic_edge(button_logic_t *state, bool pressed, uint32_t now)
{
    state->raw_pressed = pressed;
    state->since_ms = now;
    state->pending = true;
}

void button_logic_tick(button_logic_t *state, uint32_t now)
{
    /* Subtracao unsigned suporta a passagem de UINT32_MAX para zero. */
    if (!state->pending || (uint32_t)(now - state->since_ms) < BUTTON_DEBOUNCE_MS) {
        return;
    }
    state->pending = false;
    if (!state->raw_pressed) {
        state->armed = true;
        return;
    }
    if (!state->armed) {
        return;
    }
    state->armed = false;
    ++state->press_count;
    if (state->count == BUTTON_QUEUE_CAPACITY) {
        /* Preserva os eventos mais antigos; conta o pressionamento descartado. */
        ++state->lost_total;
        return;
    }
    uint32_t tail = (state->head + state->count) % BUTTON_QUEUE_CAPACITY;
    state->events[tail] = (button_event_t){state->press_count, state->since_ms};
    ++state->count;
}

bool button_logic_peek(const button_logic_t *state, button_event_t *event)
{
    if (state->count == 0U) {
        return false;
    }
    *event = state->events[state->head];
    return true;
}

void button_logic_pop(button_logic_t *state)
{
    if (state->count != 0U) {
        state->head = (state->head + 1U) % BUTTON_QUEUE_CAPACITY;
        --state->count;
    }
}
