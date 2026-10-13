#include "button.h"
#include "stm32f4xx.h"
#include "timebase.h"

static volatile button_snapshot_t latest;

void button_init(void)
{
    NVIC_DisableIRQ(EXTI0_IRQn);
    EXTI->IMR &= ~EXTI_IMR_MR0;

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    (void)RCC->AHB1ENR;
    (void)RCC->APB2ENR;

    /* USER/KEY: PA0 como entrada com pull-up; pressionado conecta a GND.
     * Altera apenas PA0, preservando PA11/PA12 usados pelo USB. */
    GPIOA->MODER &= ~GPIO_MODER_MODER0;
    GPIOA->PUPDR = (GPIOA->PUPDR & ~GPIO_PUPDR_PUPD0) | GPIO_PUPDR_PUPD0_0;

    /* Seleciona porta A para a linha EXTI0. PB0/PC0 usam a mesma linha. */
    SYSCFG->EXTICR[0] = (SYSCFG->EXTICR[0] & ~SYSCFG_EXTICR1_EXTI0) |
                       SYSCFG_EXTICR1_EXTI0_PA;
    EXTI->RTSR |= EXTI_RTSR_TR0; /* Subida: soltura. */
    EXTI->FTSR |= EXTI_FTSR_TR0; /* Descida: pressao. */
    EXTI->EMR &= ~EXTI_EMR_MR0; /* Usamos interrupcao, nao evento de hardware. */

    /* Descarta pendencias anteriores; o estado inicial nao conta como borda. */
    EXTI->PR = EXTI_PR_PR0;
    NVIC_ClearPendingIRQ(EXTI0_IRQn);
    latest.irq_count = 0U;
    latest.pressed = (GPIOA->IDR & GPIO_IDR_ID0) == 0U;
    latest.timestamp_ms = millis();

    NVIC_SetPriority(EXTI0_IRQn, 1U); /* SysTick=0, botao=1, USB=2. */
    EXTI->IMR |= EXTI_IMR_MR0;
    NVIC_EnableIRQ(EXTI0_IRQn);
}

button_snapshot_t button_snapshot(void)
{
    uint32_t enabled = NVIC_GetEnableIRQ(EXTI0_IRQn);
    NVIC_DisableIRQ(EXTI0_IRQn);
    /* volatile nao torna a copia dos tres campos atomica: protegemos a copia. */
    button_snapshot_t snapshot = latest;
    if (enabled != 0U) {
        NVIC_EnableIRQ(EXTI0_IRQn);
    }
    return snapshot;
}

void EXTI0_IRQHandler(void)
{
    if ((EXTI->PR & EXTI_PR_PR0) != 0U) {
        /* W1C: escrever 1 limpa somente PR0. Nao usar |= neste registrador. */
        EXTI->PR = EXTI_PR_PR0;
        latest.pressed = (GPIOA->IDR & GPIO_IDR_ID0) == 0U;
        latest.timestamp_ms = millis();
        ++latest.irq_count;
    }
    __DSB(); /* Garante conclusao da escrita periferica antes de sair da ISR. */
}
