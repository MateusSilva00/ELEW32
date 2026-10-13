#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "morse_logic.h"

static uint32_t symbol(morse_logic_t *state, uint32_t at, uint32_t duration)
{
    morse_result_t result;
    assert(!morse_logic_edge(state, true, at, &result));
    assert(!morse_logic_edge(state, false, at + duration, &result));
    return at + duration;
}

static void test_alphabet(void)
{
    /* Vetores de entrada para todas as letras; comparacao inclui a sequencia
     * preservada, nao apenas o caractere de saida. */
    static const struct { char letter; const char *code; } cases[] = {
        {'E', "."}, {'T', "-"}, {'I', ".."}, {'A', ".-"}, {'N', "-."}, {'M', "--"},
        {'S', "..."}, {'U', "..-"}, {'R', ".-."}, {'W', ".--"},
        {'D', "-.."}, {'K', "-.-"}, {'G', "--."}, {'O', "---"},
        {'H', "...."}, {'V', "...-"}, {'F', "..-."}, {'L', ".-.."},
        {'P', ".--."}, {'J', ".---"}, {'B', "-..."}, {'X', "-..-"},
        {'C', "-.-."}, {'Y', "-.--"}, {'Z', "--.."}, {'Q', "--.-"}
    };
    morse_logic_t state;
    morse_result_t result;
    morse_logic_init(&state);
    uint32_t now = 0U;
    for (unsigned int i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        const char *p = cases[i].code;
        while (*p) {
            now = symbol(&state, now + 100U, *p++ == '.' ? 100U : 600U);
        }
        assert(!morse_logic_tick(&state, now + 999U, &result));
        assert(morse_logic_tick(&state, now + 1000U, &result));
        assert(result.kind == MORSE_LETTER && result.letter == cases[i].letter);
        assert(strcmp(result.symbols, cases[i].code) == 0);
        assert(result.finished_at == now + 1000U);
        assert(!morse_logic_tick(&state, now + 5000U, &result));
        now += 5000U;
    }
}

static void test_thresholds_hold_duplicates(void)
{
    morse_logic_t state;
    morse_result_t result;
    for (uint32_t duration = 299U; duration <= 301U; ++duration) {
        morse_logic_init(&state);
        uint32_t release = symbol(&state, 100U, duration);
        assert(morse_logic_tick(&state, release + 1000U, &result));
        assert(result.letter == (duration == 299U ? 'E' : 'T'));
    }
    morse_logic_init(&state);
    assert(!morse_logic_tick(&state, 10000U, &result));
    assert(!morse_logic_edge(&state, false, 10U, &result)); /* Soltura sem pressao. */
    assert(!morse_logic_edge(&state, true, 100U, &result));
    assert(!morse_logic_edge(&state, true, 500U, &result)); /* Nao reinicia duracao. */
    assert(!morse_logic_tick(&state, 10100U, &result)); /* Mantido: nao encerra. */
    assert(!morse_logic_edge(&state, false, 10100U, &result));
    assert(!morse_logic_edge(&state, false, 10500U, &result)); /* Nao reinicia gap. */
    assert(!morse_logic_tick(&state, 11099U, &result));
    assert(morse_logic_tick(&state, 11100U, &result));
    assert(result.letter == 'T');
}

static void test_letter_boundary_and_delayed_tick(void)
{
    morse_logic_t state;
    morse_result_t result;
    morse_logic_init(&state);
    symbol(&state, 0U, 100U);
    /* Pressao anterior ao prazo, entregue apos debounce pela futura integracao:
     * ela precisa suspender tick enquanto valida e entregar este timestamp. */
    assert(!morse_logic_edge(&state, true, 1099U, &result));
    assert(!morse_logic_tick(&state, 2000U, &result));
    assert(!morse_logic_edge(&state, false, 2100U, &result));
    assert(morse_logic_tick(&state, 3100U, &result));
    assert(result.letter == 'A');

    for (uint32_t gap = 1000U; gap <= 1001U; ++gap) {
        morse_logic_init(&state);
        symbol(&state, 0U, 100U);
        /* Nenhum tick entre soltura e nova borda: edge finaliza E e inicia T. */
        assert(morse_logic_edge(&state, true, 100U + gap, &result));
        assert(result.letter == 'E' && result.finished_at == 1100U);
        assert(state.pressed);
        assert(!morse_logic_edge(&state, false, 400U + gap, &result));
        assert(morse_logic_tick(&state, 1400U + gap, &result));
        assert(result.letter == 'T');
    }
    morse_logic_init(&state);
    symbol(&state, 100U, 100U);
    assert(morse_logic_tick(&state, 9000U, &result));
    assert(result.finished_at == 1200U); /* Preserva prazo, nao atendimento. */
}

static void test_invalid_overflow_and_recovery(void)
{
    struct {
        uint32_t before;
        morse_logic_t state;
        uint32_t after;
    } guarded = {0};
    guarded.before = 0x12345678U;
    guarded.after = 0x87654321U;
    morse_logic_t *state = &guarded.state;
    morse_result_t result;
    morse_logic_init(state);
    uint32_t now = 0U;
    for (unsigned int i = 0U; i < 4U; ++i) { now = symbol(state, now + 50U, 300U); }
    assert(morse_logic_tick(state, now + 1000U, &result));
    assert(result.kind == MORSE_UNKNOWN && result.letter == 0);
    assert(strcmp(result.symbols, "----") == 0);
    now += 2000U;
    for (unsigned int i = 0U; i < 5U; ++i) { now = symbol(state, now + 50U, 100U); }
    assert(morse_logic_tick(state, now + 1000U, &result));
    assert(result.kind == MORSE_TOO_LONG && result.letter == 0);
    now += 2000U;
    for (unsigned int i = 0U; i < 10000U; ++i) { now = symbol(state, now + 50U, 100U); }
    assert(state->count == 4U);
    assert(guarded.before == 0x12345678U && guarded.after == 0x87654321U);
    assert(morse_logic_tick(state, now + 1000U, &result));
    assert(result.kind == MORSE_TOO_LONG && result.letter == 0);
    assert(strcmp(result.symbols, "....") == 0); /* Prefixo nunca vira H. */
    now = symbol(state, now + 2000U, 100U);
    assert(morse_logic_tick(state, now + 1000U, &result));
    assert(result.kind == MORSE_LETTER && result.letter == 'E');
    morse_result_t saved = result;
    assert(!morse_logic_tick(state, now + 2000U, &result));
    assert(result.kind == saved.kind && result.letter == saved.letter &&
           result.finished_at == saved.finished_at && strcmp(result.symbols, saved.symbols) == 0);
}

static void test_wrap_reset(void)
{
    morse_logic_t state;
    morse_result_t result;
    morse_logic_init(&state);
    uint32_t release = symbol(&state, UINT32_MAX - 100U, 300U);
    assert(release == 199U);
    assert(!morse_logic_tick(&state, 1198U, &result));
    assert(morse_logic_tick(&state, 1199U, &result));
    assert(result.letter == 'T' && result.finished_at == 1199U);
    morse_logic_init(&state);
    release = symbol(&state, UINT32_MAX - 200U, 100U);
    assert(!morse_logic_tick(&state, 898U, &result));
    assert(morse_logic_tick(&state, 899U, &result));
    assert(result.letter == 'E' && result.finished_at == 899U);
    morse_logic_init(&state);
    symbol(&state, 0U, 100U);
    morse_logic_init(&state); /* Cancela letra incompleta. */
    assert(!morse_logic_tick(&state, 2000U, &result));
    assert(!morse_logic_edge(&state, true, 2100U, &result));
    morse_logic_init(&state); /* Cancela pressao em andamento. */
    assert(!morse_logic_edge(&state, false, 3000U, &result));
    symbol(&state, 4000U, 100U);
    assert(morse_logic_tick(&state, 5100U, &result) && result.letter == 'E');
}

int main(void)
{
    test_alphabet();
    puts("PASS: 26 letras A-Z, sequencias preservadas e um resultado por letra");
    test_thresholds_hold_duplicates();
    puts("PASS: limiar 299/300/301 ms, botao mantido e bordas duplicadas");
    test_letter_boundary_and_delayed_tick();
    puts("PASS: intervalo 999/1000/1001 ms, letras sucessivas e atendimento tardio");
    test_invalid_overflow_and_recovery();
    puts("PASS: sequencia invalida, 10000 simbolos sem overflow de memoria e recuperacao");
    test_wrap_reset();
    puts("PASS: wrap uint32_t na duracao/intervalo e cancelamento por reinicializacao");
    return 0;
}
