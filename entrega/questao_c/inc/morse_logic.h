#ifndef APP_MORSE_LOGIC_H
#define APP_MORSE_LOGIC_H
#include <stdbool.h>
#include <stdint.h>

#define MORSE_DASH_MIN_MS 300U
#define MORSE_LETTER_GAP_MS 1000U
#define MORSE_MAX_SYMBOLS 4U

typedef enum { MORSE_LETTER, MORSE_UNKNOWN, MORSE_TOO_LONG } morse_result_kind_t;
typedef struct {
    morse_result_kind_t kind;
    char symbols[MORSE_MAX_SYMBOLS + 1U];
    char letter; /* A-Z se MORSE_LETTER; zero nos erros. */
    uint32_t finished_at; /* Prazo da letra, nao instante de atendimento. */
} morse_result_t;
typedef struct {
    bool pressed;
    bool overflow;
    uint8_t count;
    uint32_t pressed_at;
    uint32_t released_at;
    char symbols[MORSE_MAX_SYMBOLS + 1U];
} morse_logic_t;

/* Modulo puro; inicializa/cancela letra. Integracao deve exigir soltura
 * estavel inicial e entregar somente bordas validadas, em ordem temporal.
 * Chamador serializa acessos e usa o mesmo contador uint32_t para tudo.
 * Duracoes e atrasos devem ser menores que 2^31 ms (~24,9 dias). */
void morse_logic_init(morse_logic_t *state);

/* Retornam true SOMENTE ao produzir um resultado em *result (nao NULL).
 * result fica inalterado se false. Edge recebe timestamp da borda validada,
 * nao o fim do debounce. Uma nova pressao no prazo ou depois dele pode
 * finalizar a letra anterior e iniciar outra na mesma chamada. */
bool morse_logic_edge(morse_logic_t *state, bool pressed, uint32_t at,
                      morse_result_t *result);
/* Nao chamar tick enquanto houver borda pendente/em debounce: uma pressao
 * anterior ao prazo ainda pode pertencer a letra atual. */
bool morse_logic_tick(morse_logic_t *state, uint32_t now, morse_result_t *result);
#endif
