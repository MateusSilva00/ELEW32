#include "morse_board.h"
#include "morse_control.h"
#include "stm32f4xx.h"
#include "timebase.h"

static morse_control_t state;
static volatile bool initialized;

static uint32_t lock(void)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    return mask;
}

void morse_board_init(void)
{
    NVIC_DisableIRQ(EXTI0_IRQn);
    EXTI->IMR &= ~EXTI_IMR_MR0;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    (void)RCC->AHB1ENR;
    (void)RCC->APB2ENR;
    GPIOA->MODER &= ~GPIO_MODER_MODER0;
    GPIOA->PUPDR = (GPIOA->PUPDR & ~GPIO_PUPDR_PUPD0) | GPIO_PUPDR_PUPD0_0;
    SYSCFG->EXTICR[0] = (SYSCFG->EXTICR[0] & ~SYSCFG_EXTICR1_EXTI0) |
                       SYSCFG_EXTICR1_EXTI0_PA;
    EXTI->RTSR |= EXTI_RTSR_TR0;
    EXTI->FTSR |= EXTI_FTSR_TR0;
    EXTI->EMR &= ~EXTI_EMR_MR0;
    uint32_t mask = lock();
    EXTI->PR = EXTI_PR_PR0;
    NVIC_ClearPendingIRQ(EXTI0_IRQn);
    morse_control_init(&state, false, (GPIOA->IDR & GPIO_IDR_ID0) == 0U, millis());
    initialized = true;
    NVIC_SetPriority(EXTI0_IRQn, 1U); /* SysTick=0, EXTI=1, USB=2. */
    EXTI->IMR |= EXTI_IMR_MR0;
    NVIC_EnableIRQ(EXTI0_IRQn);
    __set_PRIMASK(mask);
}

void morse_board_tick(uint32_t now)
{
    if (!initialized) { return; }
    /* GPIO/PR apenas bloqueiam confirmacao; toda borda vem da EXTI. */
    if ((EXTI->PR & EXTI_PR_PR0) != 0U ||
        ((GPIOA->IDR & GPIO_IDR_ID0) == 0U) != state.raw_pressed) { return; }
    morse_control_tick(&state, now);
    GPIOC->BSRR = state.active && state.ready && state.stable_pressed ?
                   GPIO_BSRR_BR13 : GPIO_BSRR_BS13;
}

void morse_board_session(bool active)
{
    uint32_t mask = lock();
    morse_control_init(&state, active, (GPIOA->IDR & GPIO_IDR_ID0) == 0U, millis());
    GPIOC->BSRR = GPIO_BSRR_BS13;
    __set_PRIMASK(mask);
}

bool morse_board_peek(morse_result_t *result)
{
    uint32_t mask = lock();
    bool available = morse_control_peek(&state, result);
    __set_PRIMASK(mask);
    return available;
}

void morse_board_pop(void)
{
    uint32_t mask = lock();
    morse_control_pop(&state);
    __set_PRIMASK(mask);
}

uint32_t morse_board_lost(void)
{
    uint32_t mask = lock();
    uint32_t lost = state.lost_total;
    __set_PRIMASK(mask);
    return lost;
}

void EXTI0_IRQHandler(void)
{
    if ((EXTI->PR & EXTI_PR_PR0) != 0U) {
        uint32_t mask = lock();
        EXTI->PR = EXTI_PR_PR0; /* W1C, nunca |=. */
        morse_control_edge(&state, (GPIOA->IDR & GPIO_IDR_ID0) == 0U, millis());
        __DSB();
        __set_PRIMASK(mask);
    }
    __DSB();
}
