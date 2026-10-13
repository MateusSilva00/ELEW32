#include "rhythm_logic.h"

/* Xorshift32, como na questao (b): varia a sequencia, mas nao e fonte de
 * entropia. Zero e substituido porque seria um estado absorvente. */
static uint32_t next_random(rhythm_logic_t *state)
{
    uint32_t value = state->random_state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    state->random_state = value;
    return value;
}

static rhythm_class_t random_class(rhythm_logic_t *state)
{
    return (next_random(state) & 0x80000000U) != 0U ? RHYTHM_LONG : RHYTHM_SHORT;
}

static void new_sequence(rhythm_logic_t *state)
{
    state->steps = RHYTHM_INITIAL_STEPS;
    for (unsigned int index = 0U; index < RHYTHM_INITIAL_STEPS; ++index) {
        state->pattern[index] = random_class(state);
    }
    /* O enunciado pede intervalos curtos e longos ja na sequencia inicial. */
    if (state->pattern[0] == state->pattern[1] && state->pattern[1] == state->pattern[2]) {
        state->pattern[1] = state->pattern[0] == RHYTHM_SHORT ? RHYTHM_LONG : RHYTHM_SHORT;
    }
}

void rhythm_logic_init(rhythm_logic_t *state, uint32_t seed)
{
    *state = (rhythm_logic_t){0};
    state->phase = RHYTHM_IDLE;
    state->random_state = seed != 0U ? seed : 0x6D2B79F5U;
    new_sequence(state);
}

uint32_t rhythm_logic_nominal_ms(rhythm_class_t value)
{
    return value == RHYTHM_LONG ? RHYTHM_LONG_MS : RHYTHM_SHORT_MS;
}

bool rhythm_logic_start(rhythm_logic_t *state, uint32_t now)
{
    if (state->phase != RHYTHM_IDLE) { return false; }
    state->phase = RHYTHM_DEMO;
    state->demo_at = now + RHYTHM_LEAD_MS;
    state->received = 0U;
    state->mismatch = false;
    state->outcome = RHYTHM_PENDING;
    return true;
}

bool rhythm_logic_led(const rhythm_logic_t *state, uint32_t now)
{
    uint32_t elapsed = now - state->demo_at;
    if (state->phase != RHYTHM_DEMO || elapsed >= 0x80000000U) { return false; }
    /* steps + 1 batidas; no maximo 13 comparacoes por tick. */
    uint32_t beat = 0U;
    for (unsigned int index = 0U; index <= state->steps; ++index) {
        if (elapsed >= beat && elapsed - beat < RHYTHM_FLASH_MS) { return true; }
        if (index < state->steps) { beat += rhythm_logic_nominal_ms(state->pattern[index]); }
    }
    return false;
}

bool rhythm_logic_tick(rhythm_logic_t *state, uint32_t now, rhythm_event_t *event)
{
    if (state->phase == RHYTHM_DEMO) {
        uint32_t elapsed = now - state->demo_at;
        uint32_t length = RHYTHM_FLASH_MS;
        for (unsigned int index = 0U; index < state->steps; ++index) {
            length += rhythm_logic_nominal_ms(state->pattern[index]);
        }
        /* A entrada so e aceita depois que a ultima batida apagou. */
        if (elapsed < 0x80000000U && elapsed >= length) { state->phase = RHYTHM_WAIT; }
        return false;
    }
    uint32_t elapsed = now - state->last_click_at;
    if (state->phase != RHYTHM_INPUT || elapsed < RHYTHM_TIMEOUT_MS || elapsed >= 0x80000000U) {
        return false;
    }
    *event = (rhythm_event_t){0};
    event->outcome = RHYTHM_TIMEOUT;
    event->received = state->received;
    event->steps = state->steps;
    state->outcome = RHYTHM_TIMEOUT;
    state->phase = RHYTHM_DONE;
    return true;
}

bool rhythm_logic_press(rhythm_logic_t *state, uint32_t at, rhythm_event_t *event)
{
    /* Atendimento tardio: termina a demonstracao ou detecta o prazo antes. */
    if (rhythm_logic_tick(state, at, event)) { return true; }
    if (state->phase == RHYTHM_WAIT) {
        state->phase = RHYTHM_INPUT; /* Primeiro clique so marca o inicio. */
        state->last_click_at = at;
        return false;
    }
    uint32_t interval = at - state->last_click_at;
    if (state->phase != RHYTHM_INPUT || interval >= 0x80000000U) { return false; }
    *event = (rhythm_event_t){0};
    event->interval_ms = interval;
    event->measured = interval >= RHYTHM_LONG_MIN_MS ? RHYTHM_LONG : RHYTHM_SHORT;
    event->expected = state->pattern[state->received];
    event->steps = state->steps;
    event->received = ++state->received;
    state->last_click_at = at;
    if (event->measured != event->expected) { state->mismatch = true; }
    /* Tentativa so termina no ultimo intervalo, mostrando todos os erros. */
    event->outcome = RHYTHM_PENDING;
    if (state->received == state->steps) {
        event->outcome = state->mismatch ? RHYTHM_WRONG : RHYTHM_CORRECT;
        state->outcome = event->outcome;
        state->phase = RHYTHM_DONE;
    }
    return true;
}

bool rhythm_logic_next(rhythm_logic_t *state)
{
    if (state->phase != RHYTHM_DONE) { return false; }
    if (state->outcome == RHYTHM_CORRECT) {
        if (state->steps < RHYTHM_MAX_STEPS) {
            /* Mantem os trechos anteriores e acrescenta um novo ao final. */
            state->pattern[state->steps++] = random_class(state);
        } else {
            new_sequence(state);
        }
    }
    state->phase = RHYTHM_IDLE;
    return true;
}
