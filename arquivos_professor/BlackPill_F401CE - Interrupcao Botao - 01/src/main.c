#include "stm32f4xx.h"
#include <stdint.h>

#define BUTTON_PIN  0U      // Botao USER da BlackPill: PA0
#define LED_PIN     13U     // LED onboard: PC13 (ativo em nivel baixo)

static void led_init(void)
{
    // Habilita o clock do GPIOC
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

    // PC13 como saida (MODER13 = 01)
    GPIOC->MODER &= ~(3U << (LED_PIN * 2U));
    GPIOC->MODER |=  (1U << (LED_PIN * 2U));

    // Push-pull e sem pull-up/pull-down
    GPIOC->OTYPER &= ~(1U << LED_PIN);
    GPIOC->PUPDR  &= ~(3U << (LED_PIN * 2U));

    // LED apagado inicialmente (ativo em nivel baixo)
    GPIOC->BSRR = (1U << LED_PIN);
}

static void button_interrupt_init(void)
{
    // Habilita o clock do GPIOA
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // PA0 como entrada (MODER0 = 00)
    GPIOA->MODER &= ~(3U << (BUTTON_PIN * 2U));

    // Pull-up interno em PA0
    // Solto:       PA0 = 1
    // Pressionado: PA0 = 0
    GPIOA->PUPDR &= ~(3U << (BUTTON_PIN * 2U));
    GPIOA->PUPDR |=  (1U << (BUTTON_PIN * 2U));

    // Habilita SYSCFG para fazer o roteamento GPIO -> EXTI
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // PA0 conectado a EXTI0
    SYSCFG->EXTICR[0] &= ~(0xFU << 0U);

    // Habilita a linha EXTI0
    EXTI->IMR |= EXTI_IMR_MR0;

    // Pressionar o botao gera borda de descida: 1 -> 0
    EXTI->RTSR &= ~EXTI_RTSR_TR0;
    EXTI->FTSR |=  EXTI_FTSR_TR0;

    // Limpa eventual pendencia anterior
    EXTI->PR = EXTI_PR_PR0;

    // Habilita a interrupcao EXTI0 no NVIC
    NVIC_SetPriority(EXTI0_IRQn, 5U);
    NVIC_EnableIRQ(EXTI0_IRQn);
}

// Handler da interrupcao externa EXTI0.
// O toggle do LED acontece DIRETAMENTE dentro da ISR.
void EXTI0_IRQHandler(void)
{
    if ((EXTI->PR & EXTI_PR_PR0) != 0U)
    {
        // Limpa a pendencia da EXTI0 escrevendo 1 no bit correspondente
        EXTI->PR = EXTI_PR_PR0;

        // Toggle direto do LED onboard
        GPIOC->ODR ^= (1U << LED_PIN);
    }
}

int main(void)
{
    led_init();
    button_interrupt_init();

    while (1)
    {
        // Nao existe polling do botao.
        // A CPU pode dormir e sera acordada pela interrupcao.
        __WFI();
    }
}
