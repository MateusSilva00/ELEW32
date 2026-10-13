#ifndef APP_REACTION_LOGIC_H
#define APP_REACTION_LOGIC_H

#include <stdbool.h>
#include <stdint.h>

#define REACTION_WAIT_MIN_MS 2000U
#define REACTION_WAIT_MAX_MS 5000U

typedef enum {
    REACTION_IDLE,
    REACTION_WAITING,
    REACTION_STIMULUS,
    REACTION_RESULT,
    REACTION_EARLY
} reaction_phase_t;

typedef struct {
    reaction_phase_t phase;
    uint32_t random_state;
    uint32_t wait_started_at;
    uint32_t wait_ms;
    uint32_t stimulus_at;
    uint32_t reaction_ms;
} reaction_logic_t;

/* Modulo puro: nao acessa GPIO, USB nem SysTick. Chamador serializa os acessos.
 * Timestamps pertencem ao mesmo contador uint32_t de milissegundos; cada
 * rodada e atraso de entrega devem durar menos de 2^31 ms (~24,9 dias).
 * Pressionamentos entregues devem estar validados e em ordem temporal. */
void reaction_logic_init(reaction_logic_t *state, uint32_t seed);

/* Inicia somente em IDLE. Integracao deve exigir botao solto estavel e
 * anunciar prontidao antes de chamar. Sorteia uma espera entre 2 e 5 s. */
bool reaction_logic_start(reaction_logic_t *state, uint32_t now);

/* Consulta nao acende LED nem muda estado; prazo vencido nao e estimulo. */
bool reaction_logic_wait_elapsed(const reaction_logic_t *state, uint32_t now);

/* Chamar ao acionar o LED, na mesma secao critica do GPIO e com o timestamp
 * real do acionamento. Aceita apenas WAITING com prazo vencido. */
bool reaction_logic_stimulus(reaction_logic_t *state, uint32_t now);

/* Recebe o timestamp da borda validada, nao o fim do debounce. Retorna true
 * somente quando encerra uma rodada em EARLY ou RESULT. A igualdade com
 * stimulus_at resulta em 0 ms; a ordem dentro do mesmo ms exige cuidado na
 * integracao (nao emitir estimulo enquanto ha pressao anterior pendente). */
bool reaction_logic_press(reaction_logic_t *state, uint32_t pressed_at);

/* Libera resultado ou cancela rodada; preserva a sequencia pseudoaleatoria.
 * Nao inicia automaticamente: integrar soltura estavel e politica de USB. */
void reaction_logic_reset(reaction_logic_t *state);

#endif
