#include "timebase.h"
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "morse_board.h"

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
    /* ISR curta: debounce/tempo/LED; sem formatacao ou envio USB. */
    NVIC_SetPriority(SysTick_IRQn, 0U);
    return true;
}

uint32_t millis(void)
{
    return milliseconds;
}

/* Os timeouts do PCD/HAL consultam o mesmo contador da aplicacao. */
uint32_t HAL_GetTick(void)
{
    return millis();
}

/* A aplicacao ja inicializa SysTick; HAL nao deve criar outra base de tempo. */
HAL_StatusTypeDef HAL_InitTick(uint32_t priority)
{
    (void)priority;
    return HAL_OK;
}

/* Este simbolo substitui o alias weak do startup no vetor de excecao 15. */
void SysTick_Handler(void)
{
    ++milliseconds;
    morse_board_tick(milliseconds);
}
