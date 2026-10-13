#include "reaction_logic.h"

/* Xorshift32: suficiente para variar a espera deste exercicio. Nao e fonte
 * de entropia. A mesma semente reproduz a mesma sequencia; zero e substituido
 * porque seria um estado absorvente do gerador. */
static uint32_t next_random(reaction_logic_t *state)
{
    uint32_t value = state->random_state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    state->random_state = value;
    return value;
}

void reaction_logic_init(reaction_logic_t *state, uint32_t seed)
{
    *state = (reaction_logic_t){0};
    state->phase = REACTION_IDLE;
    state->random_state = seed != 0U ? seed : 0x6D2B79F5U;
}

bool reaction_logic_start(reaction_logic_t *state, uint32_t now)
{
    if (state->phase != REACTION_IDLE) {
        return false;
    }
    state->wait_ms = REACTION_WAIT_MIN_MS +
        next_random(state) % (REACTION_WAIT_MAX_MS - REACTION_WAIT_MIN_MS + 1U);
    state->wait_started_at = now;
    state->stimulus_at = 0U;
    state->reaction_ms = 0U;
    state->phase = REACTION_WAITING;
    return true;
}

bool reaction_logic_wait_elapsed(const reaction_logic_t *state, uint32_t now)
{
    uint32_t elapsed = now - state->wait_started_at;
    return state->phase == REACTION_WAITING &&
           elapsed < 0x80000000U && elapsed >= state->wait_ms;
}

bool reaction_logic_stimulus(reaction_logic_t *state, uint32_t now)
{
    if (!reaction_logic_wait_elapsed(state, now)) {
        return false;
    }
    state->stimulus_at = now;
    state->phase = REACTION_STIMULUS;
    return true;
}

bool reaction_logic_press(reaction_logic_t *state, uint32_t pressed_at)
{
    if (state->phase != REACTION_WAITING && state->phase != REACTION_STIMULUS) {
        return false;
    }
    /* Evento anterior ao inicio da rodada nao pode contaminar a atual.
     * Com duracoes menores que meio ciclo, unsigned distingue passado/futuro. */
    uint32_t since_start = pressed_at - state->wait_started_at;
    if (since_start >= 0x80000000U) {
        return false;
    }
    if (state->phase == REACTION_WAITING ||
        since_start < (uint32_t)(state->stimulus_at - state->wait_started_at)) {
        state->phase = REACTION_EARLY;
    } else {
        state->reaction_ms = pressed_at - state->stimulus_at;
        state->phase = REACTION_RESULT;
    }
    return true;
}

void reaction_logic_reset(reaction_logic_t *state)
{
    state->phase = REACTION_IDLE;
    state->wait_started_at = 0U;
    state->wait_ms = 0U;
    state->stimulus_at = 0U;
    state->reaction_ms = 0U;
}
