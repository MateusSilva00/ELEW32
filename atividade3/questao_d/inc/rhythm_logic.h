#ifndef APP_RHYTHM_LOGIC_H
#define APP_RHYTHM_LOGIC_H
#include <stdbool.h>
#include <stdint.h>

#define RHYTHM_SHORT_MS 300U     /* Intervalo curto demonstrado no LED. */
#define RHYTHM_LONG_MS 900U      /* Intervalo longo demonstrado no LED. */
#define RHYTHM_LONG_MIN_MS 600U  /* Classificacao: curto < 600 ms <= longo. */
#define RHYTHM_TIMEOUT_MS 2000U  /* 2 s sem clique encerra a tentativa. */
#define RHYTHM_FLASH_MS 100U     /* Duracao de cada batida no LED. */
#define RHYTHM_LEAD_MS 1000U     /* Pausa entre o inicio da rodada e a 1a batida. */
#define RHYTHM_INITIAL_STEPS 3U
#define RHYTHM_MAX_STEPS 12U

typedef enum { RHYTHM_SHORT, RHYTHM_LONG } rhythm_class_t;
typedef enum { RHYTHM_IDLE, RHYTHM_DEMO, RHYTHM_WAIT, RHYTHM_INPUT, RHYTHM_DONE } rhythm_phase_t;
/* PENDING: faltam intervalos. TIMEOUT: evento sem intervalo novo. */
typedef enum { RHYTHM_PENDING, RHYTHM_CORRECT, RHYTHM_WRONG, RHYTHM_TIMEOUT } rhythm_outcome_t;

typedef struct {
    rhythm_outcome_t outcome;
    uint8_t received; /* Intervalos medidos ate este evento, inclusive. */
    uint8_t steps;
    rhythm_class_t measured;
    rhythm_class_t expected;
    uint32_t interval_ms;
} rhythm_event_t;

typedef struct {
    rhythm_phase_t phase;
    rhythm_outcome_t outcome; /* Ultima tentativa encerrada. */
    uint32_t random_state;
    uint8_t steps;
    uint8_t received;
    bool mismatch;
    rhythm_class_t pattern[RHYTHM_MAX_STEPS];
    uint32_t demo_at;
    uint32_t last_click_at;
} rhythm_logic_t;

/* Modulo puro: nao acessa GPIO, USB nem SysTick. Chamador serializa acessos.
 * Timestamps do mesmo contador uint32_t; atrasos menores que 2^31 ms.
 * Cria sequencia de 3 trechos com curto e longo; fase IDLE. */
void rhythm_logic_init(rhythm_logic_t *state, uint32_t seed);

/* Somente em IDLE: demonstra a sequencia a partir de now + RHYTHM_LEAD_MS. */
bool rhythm_logic_start(rhythm_logic_t *state, uint32_t now);

/* Estado do LED durante a demonstracao; false fora de DEMO. */
bool rhythm_logic_led(const rhythm_logic_t *state, uint32_t now);

uint32_t rhythm_logic_nominal_ms(rhythm_class_t value);

/* Retornam true SOMENTE ao produzir evento em *event (nao NULL); event fica
 * inalterado se false. Press recebe o timestamp da borda de pressao validada.
 * Pressoes em DEMO, IDLE ou DONE sao ignoradas. Uma pressao 2 s ou mais apos
 * o clique anterior produz TIMEOUT e nao conta como intervalo. */
bool rhythm_logic_press(rhythm_logic_t *state, uint32_t at, rhythm_event_t *event);
/* Nao chamar tick com borda em debounce: uma pressao anterior ao prazo
 * ainda pertence a tentativa. Tick encerra a demonstracao e detecta TIMEOUT. */
bool rhythm_logic_tick(rhythm_logic_t *state, uint32_t now, rhythm_event_t *event);

/* Somente em DONE: correta acrescenta um trecho (ou reinicia com 3 apos o
 * maximo); incorreta mantem a mesma sequencia. Volta para IDLE. */
bool rhythm_logic_next(rhythm_logic_t *state);
#endif
