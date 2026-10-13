#include "rhythm_board.h"
#include "rhythm_control.h"
#include "stm32f4xx.h"
#include "timebase.h"

static rhythm_control_t state;
static volatile bool initialized;

static uint32_t lock(void)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    return mask;
}

static bool user_pressed(void)
{
    return (GPIOA->IDR & GPIO_IDR_ID0) == 0U; /* PA0 com pull-up, ativo em baixo. */
}

void rhythm_board_init(void)
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
    rhythm_control_init(&state, false, user_pressed(), millis(), 0U);
    initialized = true;
    NVIC_SetPriority(EXTI0_IRQn, 1U); /* SysTick=0, EXTI=1, USB=2. */
    EXTI->IMR |= EXTI_IMR_MR0;
    NVIC_EnableIRQ(EXTI0_IRQn);
    __set_PRIMASK(mask);
}

void rhythm_board_tick(uint32_t now)
{
    if (!initialized) { return; }
    /* GPIO/PR apenas bloqueiam confirmacao; toda borda vem da EXTI. */
    if ((EXTI->PR & EXTI_PR_PR0) == 0U && user_pressed() == state.raw_pressed) {
        rhythm_control_tick(&state, now);
    }
    /* O LED continua a demonstracao mesmo durante a validacao de uma borda. */
    GPIOC->BSRR = rhythm_control_led(&state, now) ? GPIO_BSRR_BR13 : GPIO_BSRR_BS13;
}

void rhythm_board_session(bool active)
{
    uint32_t mask = lock();
    /* Instante de abertura do terminal varia a seed; UID sozinho seria fixo. */
    uint32_t seed = millis() ^ (SysTick->VAL << 8) ^ *(volatile uint32_t *)UID_BASE;
    rhythm_control_init(&state, active, user_pressed(), millis(), seed);
    GPIOC->BSRR = GPIO_BSRR_BS13;
    __set_PRIMASK(mask);
}

rhythm_sequence_t rhythm_board_sequence(void)
{
    rhythm_sequence_t sequence;
    uint32_t mask = lock();
    sequence.steps = state.logic.steps;
    for (unsigned int index = 0U; index < RHYTHM_MAX_STEPS; ++index) {
        sequence.pattern[index] = state.logic.pattern[index];
    }
    __set_PRIMASK(mask);
    return sequence;
}

bool rhythm_board_start(void)
{
    uint32_t mask = lock();
    bool started = rhythm_control_start(&state, millis());
    __set_PRIMASK(mask);
    return started;
}

bool rhythm_board_next(void)
{
    uint32_t mask = lock();
    bool advanced = rhythm_logic_next(&state.logic);
    __set_PRIMASK(mask);
    return advanced;
}

bool rhythm_board_peek(rhythm_event_t *event)
{
    uint32_t mask = lock();
    bool available = rhythm_control_peek(&state, event);
    __set_PRIMASK(mask);
    return available;
}

void rhythm_board_pop(void)
{
    uint32_t mask = lock();
    rhythm_control_pop(&state);
    __set_PRIMASK(mask);
}

void EXTI0_IRQHandler(void)
{
    if ((EXTI->PR & EXTI_PR_PR0) != 0U) {
        uint32_t mask = lock();
        EXTI->PR = EXTI_PR_PR0; /* W1C, nunca |=. */
        rhythm_control_edge(&state, user_pressed(), millis());
        __DSB();
        __set_PRIMASK(mask);
    }
    __DSB();
}
