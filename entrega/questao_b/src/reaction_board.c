#include "reaction_board.h"
#include "reaction_control.h"
#include "stm32f4xx.h"
#include "timebase.h"

static reaction_control_t state;
static volatile bool initialized;

static uint32_t lock(void)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    return mask;
}

static bool input_quiet(void)
{
    /* Apenas intertravamento: GPIO nao cria eventos. As bordas vem da EXTI. */
    return (EXTI->PR & EXTI_PR_PR0) == 0U && (GPIOA->IDR & GPIO_IDR_ID0) != 0U;
}

void reaction_board_init(void)
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
    reaction_control_init(&state, (GPIOA->IDR & GPIO_IDR_ID0) == 0U, millis());
    initialized = true;
    NVIC_SetPriority(EXTI0_IRQn, 1U); /* SysTick=0, EXTI=1, USB=2. */
    EXTI->IMR |= EXTI_IMR_MR0;
    NVIC_EnableIRQ(EXTI0_IRQn);
    __set_PRIMASK(mask);
}

void reaction_board_tick(uint32_t now)
{
    if (!initialized) { return; }
    /* SysTick tem prioridade maior: nao valida estado enquanto a EXTI aguarda.
     * Divergencia de GPIO so bloqueia; nunca sintetiza pressao por polling. */
    if ((EXTI->PR & EXTI_PR_PR0) != 0U ||
        ((GPIOA->IDR & GPIO_IDR_ID0) == 0U) != state.raw_pressed) { return; }
    reaction_control_tick(&state, now);
    if (state.logic.phase == REACTION_RESULT || state.logic.phase == REACTION_EARLY) {
        GPIOC->BSRR = GPIO_BSRR_BS13;
    } else if (input_quiet() && reaction_control_stimulus(&state, now)) {
        /* Nenhuma IRQ pode preemptar SysTick (prioridade 0). Timestamp e GPIO
         * ficam no mesmo atendimento; USB nao define o inicio da medicao. */
        GPIOC->BSRR = GPIO_BSRR_BR13;
        __DSB();
    }
}

void reaction_board_new_session(void)
{
    uint32_t mask = lock();
    /* Instante de abertura do terminal varia a seed; UID sozinho seria fixo. */
    uint32_t seed = millis() ^ (SysTick->VAL << 8) ^ *(volatile uint32_t *)UID_BASE;
    reaction_logic_init(&state.logic, seed);
    GPIOC->BSRR = GPIO_BSRR_BS13;
    __set_PRIMASK(mask);
}

void reaction_board_reset(void)
{
    uint32_t mask = lock();
    reaction_logic_reset(&state.logic);
    GPIOC->BSRR = GPIO_BSRR_BS13;
    __set_PRIMASK(mask);
}

bool reaction_board_start(void)
{
    uint32_t mask = lock();
    bool started = input_quiet() && reaction_control_start(&state, millis());
    __set_PRIMASK(mask);
    return started;
}

reaction_status_t reaction_board_status(void)
{
    uint32_t mask = lock();
    reaction_status_t result = {state.logic.phase, state.logic.reaction_ms};
    __set_PRIMASK(mask);
    return result;
}

void EXTI0_IRQHandler(void)
{
    if ((EXTI->PR & EXTI_PR_PR0) != 0U) {
        uint32_t mask = lock();
        EXTI->PR = EXTI_PR_PR0; /* W1C, nunca |=. */
        reaction_control_edge(&state, (GPIOA->IDR & GPIO_IDR_ID0) == 0U, millis());
        __DSB();
        __set_PRIMASK(mask);
    }
    __DSB();
}
