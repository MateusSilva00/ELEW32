#include "rhythm_control.h"

void rhythm_control_init(rhythm_control_t *state, bool active, bool pressed,
                         uint32_t now, uint32_t seed)
{
    *state = (rhythm_control_t){0};
    state->active = active;
    rhythm_logic_init(&state->logic, seed);
    rhythm_control_edge(state, pressed, now);
}

void rhythm_control_edge(rhythm_control_t *state, bool pressed, uint32_t now)
{
    state->raw_pressed = pressed;
    state->pending = true;
    state->edge_at = now;
}

static void enqueue(rhythm_control_t *state, const rhythm_event_t *event)
{
    if (state->count == RHYTHM_QUEUE_CAPACITY) { return; } /* Ver capacidade no .h. */
    uint32_t tail = (state->head + state->count) % RHYTHM_QUEUE_CAPACITY;
    state->events[tail] = *event;
    ++state->count;
}

void rhythm_control_tick(rhythm_control_t *state, uint32_t now)
{
    if (!state->active) { return; }
    rhythm_event_t event;
    if (state->pending) {
        /* Suspende tambem o prazo de 2 s enquanto valida uma borda. */
        if ((uint32_t)(now - state->edge_at) < RHYTHM_DEBOUNCE_MS) { return; }
        state->pending = false;
        if (!state->ready) {
            if (!state->raw_pressed) { state->ready = true; }
        } else if (state->raw_pressed != state->stable_pressed) {
            state->stable_pressed = state->raw_pressed;
            /* Somente a pressao e um clique; entrega a borda, nao a confirmacao. */
            if (state->stable_pressed &&
                rhythm_logic_press(&state->logic, state->edge_at, &event)) {
                enqueue(state, &event);
            }
        }
    }
    if (state->ready && rhythm_logic_tick(&state->logic, now, &event)) {
        enqueue(state, &event);
    }
}

bool rhythm_control_led(const rhythm_control_t *state, uint32_t now)
{
    if (!state->active) { return false; }
    if (state->logic.phase == RHYTHM_DEMO) { return rhythm_logic_led(&state->logic, now); }
    return state->ready && state->stable_pressed; /* Retorno visual do clique. */
}

bool rhythm_control_start(rhythm_control_t *state, uint32_t now)
{
    return state->active && rhythm_logic_start(&state->logic, now);
}

bool rhythm_control_peek(const rhythm_control_t *state, rhythm_event_t *event)
{
    if (state->count == 0U) { return false; }
    *event = state->events[state->head];
    return true;
}

void rhythm_control_pop(rhythm_control_t *state)
{
    if (state->count != 0U) {
        state->head = (state->head + 1U) % RHYTHM_QUEUE_CAPACITY;
        --state->count;
    }
}
