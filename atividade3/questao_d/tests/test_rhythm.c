#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "rhythm_logic.h"
void test_rhythm_integration(void);

static const rhythm_class_t S = RHYTHM_SHORT;
static const rhythm_class_t L = RHYTHM_LONG;

/* Sequencia fixa para os testes: curto, longo, curto. */
static void fixed(rhythm_logic_t *state, uint32_t now)
{
    rhythm_logic_init(state, 1U);
    state->pattern[0] = S;
    state->pattern[1] = L;
    state->pattern[2] = S;
    assert(rhythm_logic_start(state, now));
}

/* Termina a demonstracao e retorna o instante em que a entrada foi liberada. */
static uint32_t finish_demo(rhythm_logic_t *state)
{
    rhythm_event_t event;
    uint32_t end = state->demo_at + RHYTHM_FLASH_MS;
    for (unsigned int index = 0U; index < state->steps; ++index) {
        end += rhythm_logic_nominal_ms(state->pattern[index]);
    }
    assert(!rhythm_logic_tick(state, end - 1U, &event) && state->phase == RHYTHM_DEMO);
    assert(!rhythm_logic_tick(state, end, &event) && state->phase == RHYTHM_WAIT);
    return end;
}

static rhythm_event_t press(rhythm_logic_t *state, uint32_t at)
{
    rhythm_event_t event;
    assert(rhythm_logic_press(state, at, &event));
    return event;
}

static void test_initial_sequences(void)
{
    rhythm_logic_t state;
    unsigned int long_first = 0U;
    for (uint32_t seed = 0U; seed < 20000U; ++seed) {
        rhythm_logic_init(&state, seed);
        assert(state.phase == RHYTHM_IDLE && state.steps == RHYTHM_INITIAL_STEPS);
        unsigned int longs = 0U;
        for (unsigned int index = 0U; index < 3U; ++index) {
            assert(state.pattern[index] == S || state.pattern[index] == L);
            longs += state.pattern[index] == L;
        }
        assert(longs == 1U || longs == 2U); /* Sempre curto e longo. */
        long_first += state.pattern[0] == L;
    }
    assert(long_first > 8000U && long_first < 12000U); /* Varia com a semente. */
    rhythm_logic_t again;
    rhythm_logic_init(&state, 1234U);
    rhythm_logic_init(&again, 1234U);
    assert(memcmp(state.pattern, again.pattern, sizeof(state.pattern)) == 0);
}

static void test_demo_led(void)
{
    rhythm_logic_t state;
    rhythm_event_t event;
    rhythm_logic_init(&state, 1U);
    assert(!rhythm_logic_led(&state, 0U));
    assert(!rhythm_logic_press(&state, 10U, &event)); /* IDLE ignora. */
    fixed(&state, 1000U); /* Batidas em 2000, 2300, 3200 e 3500. */
    assert(!rhythm_logic_start(&state, 1000U));
    const uint32_t beats[] = {2000U, 2300U, 3200U, 3500U};
    assert(!rhythm_logic_led(&state, 1999U));
    for (unsigned int index = 0U; index < 4U; ++index) {
        assert(rhythm_logic_led(&state, beats[index]));
        assert(rhythm_logic_led(&state, beats[index] + 99U));
        assert(!rhythm_logic_led(&state, beats[index] + 100U));
    }
    assert(!rhythm_logic_led(&state, 2250U));
    /* Cliques durante a demonstracao sao ignorados. */
    assert(!rhythm_logic_press(&state, 2300U, &event));
    assert(!rhythm_logic_press(&state, 3599U, &event) && state.phase == RHYTHM_DEMO);
    assert(finish_demo(&state) == 3600U);
    assert(!rhythm_logic_led(&state, 3500U));
    /* Sem primeiro clique nao ha prazo. */
    assert(!rhythm_logic_tick(&state, 1000000U, &event) && state.phase == RHYTHM_WAIT);
}

static void test_correct_and_thresholds(void)
{
    rhythm_logic_t state;
    rhythm_event_t event;
    fixed(&state, 0U);
    uint32_t at = finish_demo(&state) + 500U;
    assert(!rhythm_logic_press(&state, at, &event) && state.phase == RHYTHM_INPUT);
    event = press(&state, at + 599U);
    assert(event.received == 1U && event.steps == 3U && event.interval_ms == 599U);
    assert(event.measured == S && event.expected == S && event.outcome == RHYTHM_PENDING);
    event = press(&state, at + 599U + 600U);
    assert(event.interval_ms == 600U && event.measured == L && event.expected == L);
    assert(event.outcome == RHYTHM_PENDING);
    memset(&event, 0x5A, sizeof(event));
    rhythm_event_t untouched = event;
    assert(!rhythm_logic_tick(&state, at + 1199U + 1999U, &event));
    assert(memcmp(&event, &untouched, sizeof(event)) == 0);
    event = press(&state, at + 1199U + 1999U); /* 1999 ms ainda e intervalo. */
    assert(event.interval_ms == 1999U && event.measured == L && event.expected == S);
    assert(event.received == 3U && event.outcome == RHYTHM_WRONG);
    assert(state.phase == RHYTHM_DONE && state.outcome == RHYTHM_WRONG);
    assert(!rhythm_logic_press(&state, at + 4000U, &event)); /* Apos DONE. */

    fixed(&state, 0U);
    at = finish_demo(&state);
    assert(!rhythm_logic_press(&state, at, &event)); /* Clique logo apos a demo. */
    event = press(&state, at + 300U);
    event = press(&state, at + 1200U);
    event = press(&state, at + 1500U);
    assert(event.outcome == RHYTHM_CORRECT && state.outcome == RHYTHM_CORRECT);
}

static void test_wrong_reports_every_interval(void)
{
    rhythm_logic_t state;
    fixed(&state, 0U);
    uint32_t at = finish_demo(&state);
    rhythm_event_t event;
    assert(!rhythm_logic_press(&state, at, &event));
    event = press(&state, at + 900U); /* Esperado curto. */
    assert(event.measured == L && event.expected == S && event.outcome == RHYTHM_PENDING);
    event = press(&state, at + 1800U);
    assert(event.measured == L && event.expected == L && event.outcome == RHYTHM_PENDING);
    event = press(&state, at + 2100U);
    assert(event.measured == S && event.expected == S && event.outcome == RHYTHM_WRONG);
}

static void test_timeout(void)
{
    rhythm_logic_t state;
    rhythm_event_t event;
    fixed(&state, 0U);
    uint32_t at = finish_demo(&state);
    assert(!rhythm_logic_press(&state, at, &event));
    event = press(&state, at + 300U);
    assert(!rhythm_logic_tick(&state, at + 300U + 1999U, &event));
    assert(rhythm_logic_tick(&state, at + 300U + 2000U, &event));
    assert(event.outcome == RHYTHM_TIMEOUT && event.received == 1U && event.steps == 3U);
    assert(event.interval_ms == 0U && state.phase == RHYTHM_DONE);
    assert(!rhythm_logic_tick(&state, at + 9000U, &event)); /* Um unico evento. */

    /* Prazo vencido sem tick: o clique atrasado gera TIMEOUT e nao conta. */
    fixed(&state, 0U);
    at = finish_demo(&state);
    assert(!rhythm_logic_press(&state, at, &event));
    event = press(&state, at + 2000U);
    assert(event.outcome == RHYTHM_TIMEOUT && event.received == 0U);
    assert(state.received == 0U && state.phase == RHYTHM_DONE);
}

static void test_next_levels(void)
{
    rhythm_logic_t state;
    rhythm_event_t event;
    fixed(&state, 0U);
    assert(!rhythm_logic_next(&state)); /* Somente em DONE. */
    uint32_t at = finish_demo(&state);
    assert(!rhythm_logic_press(&state, at, &event));
    press(&state, at + 900U);
    press(&state, at + 1800U);
    press(&state, at + 2100U);
    assert(rhythm_logic_next(&state) && state.phase == RHYTHM_IDLE);
    assert(state.steps == 3U && state.pattern[0] == S && state.pattern[1] == L &&
           state.pattern[2] == S); /* Incorreta repete a mesma sequencia. */

    rhythm_class_t previous[RHYTHM_MAX_STEPS];
    for (unsigned int level = 3U; level <= RHYTHM_MAX_STEPS; ++level) {
        assert(state.steps == level);
        memcpy(previous, state.pattern, sizeof(previous));
        assert(rhythm_logic_start(&state, at));
        at = finish_demo(&state);
        assert(!rhythm_logic_press(&state, at, &event));
        for (unsigned int index = 0U; index < state.steps; ++index) {
            at += rhythm_logic_nominal_ms(state.pattern[index]);
            event = press(&state, at);
        }
        assert(event.outcome == RHYTHM_CORRECT);
        assert(rhythm_logic_next(&state));
        if (level < RHYTHM_MAX_STEPS) {
            assert(state.steps == level + 1U);
            assert(memcmp(previous, state.pattern, level * sizeof(previous[0])) == 0);
        }
    }
    /* Depois do nivel maximo, nova sequencia inicial com curto e longo. */
    assert(state.steps == RHYTHM_INITIAL_STEPS && state.phase == RHYTHM_IDLE);
    assert(!(state.pattern[0] == state.pattern[1] && state.pattern[1] == state.pattern[2]));

    /* Timeout tambem repete o nivel. */
    fixed(&state, 0U);
    at = finish_demo(&state);
    assert(!rhythm_logic_press(&state, at, &event));
    assert(rhythm_logic_tick(&state, at + 2000U, &event));
    assert(rhythm_logic_next(&state) && state.steps == 3U && state.pattern[1] == L);
}

static void test_wrap(void)
{
    rhythm_logic_t state;
    rhythm_event_t event;
    fixed(&state, UINT32_MAX - 1500U); /* Demo comeca em UINT32_MAX - 500. */
    assert(!rhythm_logic_led(&state, UINT32_MAX - 501U));
    assert(rhythm_logic_led(&state, UINT32_MAX - 500U));
    assert(!rhythm_logic_led(&state, 698U));
    assert(rhythm_logic_led(&state, 699U)); /* Terceira batida, depois do zero. */
    assert(!rhythm_logic_led(&state, 799U));
    uint32_t at = finish_demo(&state);
    assert(at == 1099U);
    fixed(&state, UINT32_MAX - 5000U);
    at = finish_demo(&state);
    at = UINT32_MAX - 100U;
    assert(!rhythm_logic_press(&state, at, &event));
    event = press(&state, at + 300U);
    assert(event.interval_ms == 300U && event.measured == S);
    assert(!rhythm_logic_tick(&state, at + 300U + 1999U, &event));
    assert(rhythm_logic_tick(&state, at + 300U + 2000U, &event));
    assert(event.outcome == RHYTHM_TIMEOUT);
}

int main(void)
{
    test_initial_sequences();
    puts("PASS: 20000 sementes com 3 trechos curtos e longos, variaveis e reproduziveis");
    test_demo_led();
    puts("PASS: LED da demonstracao 300/900 ms, batidas de 100 ms e cliques ignorados");
    test_correct_and_thresholds();
    puts("PASS: limiar 599/600 ms, intervalo 1999 ms e tentativa correta/incorreta");
    test_wrong_reports_every_interval();
    puts("PASS: tentativa incorreta mostra todos os intervalos antes do resultado");
    test_timeout();
    puts("PASS: prazo 1999/2000 ms, um unico evento e clique atrasado sem intervalo");
    test_next_levels();
    puts("PASS: niveis 3 a 12 mantendo o prefixo, repeticao apos erro e reinicio");
    test_wrap();
    puts("PASS: wrap uint32_t na demonstracao, nos intervalos e no prazo");
    test_rhythm_integration();
    return 0;
}
