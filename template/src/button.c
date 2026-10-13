#include "button.h"
#include "stm32f4xx.h"
#include "timebase.h"

static button_logic_t state;
static volatile bool initialized;

/* Secoes curtas: preservam PRIMASK e impedem acesso parcial entre main,
 * EXTI0 e SysTick. Nao fazer USB, formatacao ou espera com IRQs mascaradas. */
static uint32_t lock(void)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    return mask;
}

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
    uint32_t mask = lock();
    button_logic_init(&state, (GPIOA->IDR & GPIO_IDR_ID0) == 0U, millis());
    initialized = true;
    __set_PRIMASK(mask);

    NVIC_SetPriority(EXTI0_IRQn, 1U); /* SysTick=0, botao=1, USB=2. */
    EXTI->IMR |= EXTI_IMR_MR0;
    NVIC_EnableIRQ(EXTI0_IRQn);
}

void button_tick(uint32_t now)
{
    if (!initialized) {
        return;
    }
    /* SysTick tem prioridade maior: nao confirmar enquanto uma borda ainda
     * aguarda atendimento pela EXTI0. Conferimos tambem o nivel fisico. */
    bool pressed = (GPIOA->IDR & GPIO_IDR_ID0) == 0U;
    if ((EXTI->PR & EXTI_PR_PR0) != 0U) {
        return;
    }
    if (pressed != state.raw_pressed) {
        button_logic_edge(&state, pressed, now);
        return;
    }
    button_logic_tick(&state, now);
}

bool button_event_peek(button_event_t *event)
{
    uint32_t mask = lock();
    bool available = button_logic_peek(&state, event);
    __set_PRIMASK(mask);
    return available;
}

void button_event_pop(void)
{
    uint32_t mask = lock();
    button_logic_pop(&state);
    __set_PRIMASK(mask);
}

uint32_t button_lost_total(void)
{
    uint32_t mask = lock();
    uint32_t total = state.lost_total;
    __set_PRIMASK(mask);
    return total;
}

void EXTI0_IRQHandler(void)
{
    if ((EXTI->PR & EXTI_PR_PR0) != 0U) {
        uint32_t mask = lock();
        /* W1C: escrever 1 limpa somente PR0. Nao usar |= neste registrador. */
        EXTI->PR = EXTI_PR_PR0;
        button_logic_edge(&state, (GPIOA->IDR & GPIO_IDR_ID0) == 0U, millis());
        __DSB();
        __set_PRIMASK(mask);
    }
    __DSB(); /* Garante conclusao da escrita periferica antes de sair da ISR. */
}
