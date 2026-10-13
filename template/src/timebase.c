#include "timebase.h"
#include "stm32f4xx.h"

/* Compartilhado entre ISR e main; leitura alinhada de 32 bits e atomica no M4. */
static volatile uint32_t milliseconds;

bool timebase_init(void)
{
    if ((SystemCoreClock < 1000U) || ((SystemCoreClock % 1000U) != 0U)) {
        return false;
    }
    milliseconds = 0U;
    /* CMSIS seleciona o clock da CPU, LOAD=95999, VAL=0 e habilita a interrupcao. */
    if (SysTick_Config(SystemCoreClock / 1000U) != 0U) {
        return false;
    }
    /* ISR curta com prioridade alta; nao realiza trabalho de perifericos. */
    NVIC_SetPriority(SysTick_IRQn, 0U);
    return true;
}

uint32_t millis(void)
{
    return milliseconds;
}

/* Este simbolo substitui o alias weak do startup no vetor de excecao 15. */
void SysTick_Handler(void)
{
    ++milliseconds;
}
