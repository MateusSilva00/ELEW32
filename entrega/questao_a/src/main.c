#include "stm32f4xx.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "usb_serial.h"

// Questao (a): contador de pressionamentos por interrupcao externa,
// com timestamp e debounce feitos com o SysTick.

#define BUTTON_PIN   0U     // Botao USER da BlackPill: PA0
#define LED_PIN      13U    // LED onboard: PC13 (ativo em nivel baixo)
#define DEBOUNCE_MS  30U    // Tempo de nivel estavel para validar uma borda

// Alteradas dentro da interrupcao e lidas no programa principal.
volatile uint32_t edge_ms = 0U;      // Instante (SysTick) da ultima borda
volatile uint8_t edge_pending = 0U;  // Ha borda aguardando o debounce

static bool button_pressed = false;  // Ultimo nivel estavel validado

static void led_init(void)
{
    // Habilita o clock do GPIOC
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

    // PC13 como saida push-pull, sem pull-up/pull-down
    GPIOC->MODER &= ~(3U << (LED_PIN * 2U));
    GPIOC->MODER |=  (1U << (LED_PIN * 2U));
    GPIOC->OTYPER &= ~(1U << LED_PIN);
    GPIOC->PUPDR  &= ~(3U << (LED_PIN * 2U));

    // LED apagado (ativo em nivel baixo)
    GPIOC->BSRR = (1U << LED_PIN);
}

static void led_set(bool on)
{
    GPIOC->BSRR = on ? (1U << (LED_PIN + 16U)) : (1U << LED_PIN);
}

static void button_interrupt_init(void)
{
    // Habilita o clock do GPIOA
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // PA0 como entrada com pull-up: solto = 1, pressionado = 0
    GPIOA->MODER &= ~(3U << (BUTTON_PIN * 2U));
    GPIOA->PUPDR &= ~(3U << (BUTTON_PIN * 2U));
    GPIOA->PUPDR |=  (1U << (BUTTON_PIN * 2U));

    // SYSCFG: PA0 conectado a EXTI0
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    SYSCFG->EXTICR[0] &= ~(0xFU << 0U);

    // As duas bordas: o debounce precisa saber quando o botao foi solto
    EXTI->RTSR |= EXTI_RTSR_TR0;
    EXTI->FTSR |= EXTI_FTSR_TR0;

    // Estado inicial e limpeza de pendencias anteriores
    button_pressed = (GPIOA->IDR & (1U << BUTTON_PIN)) == 0U;
    EXTI->PR = EXTI_PR_PR0;
    EXTI->IMR |= EXTI_IMR_MR0;

    NVIC_SetPriority(EXTI0_IRQn, 1U);
    NVIC_EnableIRQ(EXTI0_IRQn);
}

// A ISR apenas registra o instante da borda.
// A validacao (debounce) fica no programa principal.
void EXTI0_IRQHandler(void)
{
    if ((EXTI->PR & EXTI_PR_PR0) != 0U)
    {
        // Escreve 1 no bit PR para limpar a pendencia
        EXTI->PR = EXTI_PR_PR0;

        edge_ms = millis();
        edge_pending = 1U;
    }
}

// Retorna true quando o botao muda de nivel de forma estavel.
// *at recebe o instante da borda que iniciou o novo nivel.
static bool button_update(uint32_t *at)
{
    bool ready;
    uint32_t edge;

    // Le e limpa a pendencia sem ser interrompido pela EXTI
    __disable_irq();
    edge = edge_ms;
    ready = (edge_pending != 0U) && ((uint32_t)(millis() - edge) >= DEBOUNCE_MS);
    if (ready)
    {
        edge_pending = 0U;
    }
    __enable_irq();

    if (!ready)
    {
        return false;
    }

    // Sem novas bordas por DEBOUNCE_MS: o nivel atual e estavel
    bool pressed = (GPIOA->IDR & (1U << BUTTON_PIN)) == 0U;

    if (pressed == button_pressed)
    {
        return false;  // Bouncing que voltou ao mesmo nivel
    }

    button_pressed = pressed;
    *at = edge;
    return true;
}

int main(void)
{
    // Configura clock, SysTick (1 ms) e USB CDC; espera o terminal abrir
    usb_serial_init();
    setvbuf(stdout, NULL, _IONBF, 0);

    led_init();
    button_interrupt_init();

    printf("Sistema iniciado...\r\n\r\n");

    uint32_t counter = 0U;

    while (1)
    {
        usb_serial_task();

        uint32_t at;

        if (button_update(&at))
        {
            // LED acompanha o botao como retorno visual
            led_set(button_pressed);

            if (button_pressed)
            {
                counter++;
                printf("Botao %lu pressionado em %lu ms\r\n",
                       (unsigned long)counter, (unsigned long)at);
            }
        }
    }
}
