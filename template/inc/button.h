#ifndef APP_BUTTON_H
#define APP_BUTTON_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t irq_count;    /* Atendimentos da EXTI0, nao cliques validos. */
    uint32_t timestamp_ms; /* Instante de atendimento, com resolucao de 1 ms. */
    bool pressed;         /* PA0 ativo em nivel baixo. */
} button_snapshot_t;

/* Chamar uma vez depois de iniciar SysTick. */
void button_init(void);
/* Copia coerente do ultimo registro; chamar somente no laco principal. */
button_snapshot_t button_snapshot(void);

#endif
