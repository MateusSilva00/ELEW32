#include "morse_control.h"

void morse_control_init(morse_control_t *state, bool active, bool pressed, uint32_t now)
{
    *state = (morse_control_t){0};
    state->active = active;
    morse_logic_init(&state->logic);
    morse_control_edge(state, pressed, now);
}

void morse_control_edge(morse_control_t *state, bool pressed, uint32_t now)
{
    state->raw_pressed = pressed;
    state->pending = true;
    state->edge_at = now;
}

static void enqueue(morse_control_t *state, const morse_result_t *result)
{
    if (state->count == MORSE_QUEUE_CAPACITY) {
        ++state->lost_total; /* Preserva os antigos, descarta o resultado novo. */
        return;
    }
    uint32_t tail = (state->head + state->count) % MORSE_QUEUE_CAPACITY;
    state->events[tail] = *result;
    ++state->count;
}

void morse_control_tick(morse_control_t *state, uint32_t now)
{
    if (!state->active) { return; }
    morse_result_t result;
    if (state->pending) {
        /* Suspende tambem o fim de letra enquanto valida uma borda. */
        if ((uint32_t)(now - state->edge_at) < MORSE_DEBOUNCE_MS) { return; }
        state->pending = false;
        if (!state->ready) {
            if (!state->raw_pressed) { state->ready = true; }
        } else if (state->raw_pressed != state->stable_pressed) {
            state->stable_pressed = state->raw_pressed;
            if (morse_logic_edge(&state->logic, state->stable_pressed,
                                 state->edge_at, &result)) {
                enqueue(state, &result);
            }
        }
    }
    if (state->ready && morse_logic_tick(&state->logic, now, &result)) {
        enqueue(state, &result);
    }
}

bool morse_control_peek(const morse_control_t *state, morse_result_t *result)
{
    if (state->count == 0U) { return false; }
    *result = state->events[state->head];
    return true;
}

void morse_control_pop(morse_control_t *state)
{
    if (state->count != 0U) {
        state->head = (state->head + 1U) % MORSE_QUEUE_CAPACITY;
        --state->count;
    }
}
