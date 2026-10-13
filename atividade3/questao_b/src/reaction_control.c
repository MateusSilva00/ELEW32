#include "reaction_control.h"

void reaction_control_init(reaction_control_t *state, bool pressed, uint32_t now)
{
    *state = (reaction_control_t){0};
    reaction_logic_init(&state->logic, 0U);
    reaction_control_edge(state, pressed, now);
}

void reaction_control_edge(reaction_control_t *state, bool pressed, uint32_t now)
{
    state->raw_pressed = pressed;
    state->pending = true;
    state->edge_at = now;
}

void reaction_control_tick(reaction_control_t *state, uint32_t now)
{
    if (!state->pending || (uint32_t)(now - state->edge_at) < REACTION_DEBOUNCE_MS) {
        return;
    }
    state->pending = false;
    if (!state->raw_pressed) {
        state->armed = true;
    } else if (state->armed) {
        state->armed = false;
        /* Entrega a borda, nao a confirmacao 30 ms depois. */
        reaction_logic_press(&state->logic, state->edge_at);
    }
}

bool reaction_control_released(const reaction_control_t *state)
{
    return state->armed && !state->pending && !state->raw_pressed;
}

bool reaction_control_start(reaction_control_t *state, uint32_t now)
{
    return reaction_control_released(state) && reaction_logic_start(&state->logic, now);
}

bool reaction_control_stimulus(reaction_control_t *state, uint32_t now)
{
    /* Uma borda anterior em validacao impede o LED, mesmo se o prazo venceu.
     * Isso tambem resolve pressao e prazo no mesmo tick de milissegundos. */
    return reaction_control_released(state) && reaction_logic_stimulus(&state->logic, now);
}
