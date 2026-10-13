#ifndef APP_BUTTON_H
#define APP_BUTTON_H

#include "button_logic.h"

/* Chamar uma vez depois de iniciar SysTick. */
void button_init(void);
/* Chamado exclusivamente pelo SysTick; nao transmite nem espera. */
void button_tick(uint32_t now);
/* Consumidor unico no main; so remover apos conclusao da transferencia USB. */
bool button_event_peek(button_event_t *event);
void button_event_pop(void);
uint32_t button_lost_total(void);

#endif
