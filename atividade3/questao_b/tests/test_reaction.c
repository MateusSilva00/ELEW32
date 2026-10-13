#include <assert.h>
#include <stdio.h>
#include "reaction_logic.h"

static uint32_t start(reaction_logic_t *state, uint32_t now)
{
    assert(reaction_logic_start(state, now));
    assert(state->phase == REACTION_WAITING);
    assert(state->wait_ms >= REACTION_WAIT_MIN_MS);
    assert(state->wait_ms <= REACTION_WAIT_MAX_MS);
    return now + state->wait_ms;
}

static void test_deadline_and_actual_stimulus(void)
{
    reaction_logic_t state;
    reaction_logic_init(&state, 42U);
    assert(state.phase == REACTION_IDLE);
    assert(!reaction_logic_press(&state, 0U));
    assert(!reaction_logic_wait_elapsed(&state, 100000U));
    assert(!reaction_logic_stimulus(&state, 100000U));
    uint32_t deadline = start(&state, 100U);
    uint32_t random_state = state.random_state;
    assert(!reaction_logic_start(&state, 200U));
    assert(state.random_state == random_state && state.wait_started_at == 100U);
    assert(!reaction_logic_wait_elapsed(&state, 99U));
    assert(!reaction_logic_stimulus(&state, deadline - 1U));
    assert(state.phase == REACTION_WAITING);
    assert(reaction_logic_wait_elapsed(&state, deadline));
    assert(state.phase == REACTION_WAITING); /* Prazo nao aciona estimulo. */
    uint32_t actual = deadline + 17U; /* Simula atraso do atendimento. */
    assert(reaction_logic_stimulus(&state, actual));
    assert(state.phase == REACTION_STIMULUS && state.stimulus_at == actual);
    assert(!reaction_logic_stimulus(&state, actual + 1U));
    assert(!reaction_logic_wait_elapsed(&state, actual + 1U));
    assert(reaction_logic_press(&state, actual + 284U));
    assert(state.phase == REACTION_RESULT && state.reaction_ms == 284U);
    assert(!reaction_logic_press(&state, actual + 300U));
    assert(!reaction_logic_start(&state, actual + 400U));
    assert(state.reaction_ms == 284U); /* Resultado fica retido para a serial. */
}

static void test_early_and_delayed_delivery(void)
{
    /* Ainda e cedo no prazo ou depois dele se o LED nao foi acionado. */
    for (unsigned int scenario = 0U; scenario < 3U; ++scenario) {
        reaction_logic_t state;
        reaction_logic_init(&state, 10U);
        uint32_t deadline = start(&state, 1000U);
        uint32_t press = scenario == 0U ? 1000U : deadline + scenario - 1U;
        assert(reaction_logic_press(&state, press));
        assert(state.phase == REACTION_EARLY);
        assert(!reaction_logic_stimulus(&state, deadline + 100U));
        assert(!reaction_logic_press(&state, deadline + 200U));
        assert(!reaction_logic_start(&state, deadline + 300U));
    }
    reaction_logic_t state;
    reaction_logic_init(&state, 10U);
    uint32_t deadline = start(&state, 1000U);
    assert(!reaction_logic_press(&state, 999U)); /* Borda de rodada antiga. */
    assert(state.phase == REACTION_WAITING);
    assert(reaction_logic_stimulus(&state, deadline));
    assert(!reaction_logic_press(&state, 999U));
    /* Entrega apos acender: a borda ocorreu 1 ms ANTES do estimulo. */
    assert(reaction_logic_press(&state, deadline - 1U));
    assert(state.phase == REACTION_EARLY);
}

static void test_zero_and_wrap(void)
{
    reaction_logic_t state;
    reaction_logic_init(&state, 1U);
    uint32_t deadline = start(&state, UINT32_MAX - 1000U);
    assert(deadline < 5000U); /* Espera atravessa a volta do contador. */
    assert(!reaction_logic_wait_elapsed(&state, deadline - 1U));
    assert(reaction_logic_stimulus(&state, deadline));
    assert(reaction_logic_press(&state, deadline));
    assert(state.phase == REACTION_RESULT && state.reaction_ms == 0U);

    reaction_logic_reset(&state);
    start(&state, UINT32_MAX - 6000U);
    assert(reaction_logic_stimulus(&state, UINT32_MAX - 100U));
    assert(reaction_logic_press(&state, 183U));
    assert(state.phase == REACTION_RESULT && state.reaction_ms == 284U);

    reaction_logic_reset(&state);
    start(&state, UINT32_MAX - 6000U);
    assert(reaction_logic_stimulus(&state, 10U));
    assert(reaction_logic_press(&state, UINT32_MAX - 5U));
    assert(state.phase == REACTION_EARLY); /* Entrega tardia cruza o wrap. */
}

static void test_reset_and_sequence(void)
{
    reaction_logic_t state, reference;
    reaction_logic_init(&state, 0U);
    reaction_logic_init(&reference, 0U);
    uint32_t previous = 0U;
    unsigned int changes = 0U;
    bool seen_min = false, seen_max = false;
    for (unsigned int trial = 0U; trial < 50000U; ++trial) {
        uint32_t deadline = start(&state, trial * 10000U);
        start(&reference, trial * 10000U);
        assert(state.wait_ms == reference.wait_ms); /* Reproduzivel por seed. */
        assert(state.random_state != 0U);
        if (state.wait_ms != previous) { ++changes; }
        previous = state.wait_ms;
        seen_min |= state.wait_ms == REACTION_WAIT_MIN_MS;
        seen_max |= state.wait_ms == REACTION_WAIT_MAX_MS;
        if (trial % 3U == 0U) {
            assert(reaction_logic_stimulus(&state, deadline));
            assert(reaction_logic_press(&state, deadline + 1U));
        } else if (trial % 3U == 1U) {
            assert(reaction_logic_press(&state, deadline - 1U));
        } /* Terceiro caso cancela uma espera em andamento. */
        uint32_t random_state = state.random_state;
        reaction_logic_reset(&state);
        reaction_logic_reset(&reference);
        assert(state.phase == REACTION_IDLE && state.random_state == random_state);
        assert(state.wait_ms == 0U && state.stimulus_at == 0U && state.reaction_ms == 0U);
    }
    assert(changes > 100U && seen_min && seen_max);
    start(&state, 0U);
    assert(reaction_logic_stimulus(&state, state.wait_ms));
    reaction_logic_reset(&state); /* Cancelamento com LED ja acionado. */
    assert(!reaction_logic_press(&state, 6000U));
    start(&state, 7000U);
    assert(!reaction_logic_press(&state, 6999U));
}

int main(void)
{
    test_deadline_and_actual_stimulus();
    puts("PASS: prazo, estimulo real e resultado de 284 ms retido");
    test_early_and_delayed_delivery();
    puts("PASS: antecipacao, entrega tardia e rejeicao de eventos antigos");
    test_zero_and_wrap();
    puts("PASS: reacao de 0 ms e wrap de uint32_t na espera/reacao/antecipacao");
    test_reset_and_sequence();
    puts("PASS: reset/cancelamento e 50000 rodadas com espera entre 2000 e 5000 ms");
    return 0;
}
