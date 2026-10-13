#include "morse_logic.h"
#include <string.h>

/* A-Z: ITU-R M.1677-1, Anexo 1, parte I, 1.1.1. Somente letras sem acento.
 * Origem e escolhas de temporizacao documentadas em docs/logica.md. */
static const char *const codes[] = {
    ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..",
    ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.",
    "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--.."
};

void morse_logic_init(morse_logic_t *state)
{
    *state = (morse_logic_t){0};
}

bool morse_logic_tick(morse_logic_t *state, uint32_t now, morse_result_t *result)
{
    uint32_t elapsed = now - state->released_at;
    if (state->pressed || state->count == 0U || elapsed < MORSE_LETTER_GAP_MS ||
        elapsed >= 0x80000000U) {
        return false;
    }
    *result = (morse_result_t){0};
    memcpy(result->symbols, state->symbols, sizeof(result->symbols));
    result->finished_at = state->released_at + MORSE_LETTER_GAP_MS;
    result->kind = state->overflow ? MORSE_TOO_LONG : MORSE_UNKNOWN;
    if (!state->overflow) {
        for (unsigned int index = 0U; index < sizeof(codes) / sizeof(codes[0]); ++index) {
            if (strcmp(state->symbols, codes[index]) == 0) {
                result->kind = MORSE_LETTER;
                result->letter = (char)('A' + index);
                break;
            }
        }
    }
    /* Resultado e uma copia; a proxima letra nao altera o evento entregue. */
    state->count = 0U;
    state->overflow = false;
    memset(state->symbols, 0, sizeof(state->symbols));
    return true;
}

bool morse_logic_edge(morse_logic_t *state, bool pressed, uint32_t at,
                      morse_result_t *result)
{
    if (pressed == state->pressed) { return false; }
    if (pressed) {
        /* Funciona mesmo se o main/SysTick nao consultou exatamente no prazo. */
        bool emitted = morse_logic_tick(state, at, result);
        state->pressed = true;
        state->pressed_at = at;
        return emitted;
    }
    uint32_t duration = at - state->pressed_at;
    if (duration >= 0x80000000U) { return false; }
    state->pressed = false;
    state->released_at = at;
    if (state->count < MORSE_MAX_SYMBOLS) {
        state->symbols[state->count++] = duration < MORSE_DASH_MIN_MS ? '.' : '-';
        state->symbols[state->count] = '\0';
    } else {
        /* Satura sem sobrescrever memoria ou decodificar apenas o prefixo. */
        state->overflow = true;
    }
    return false;
}
