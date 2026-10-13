#include "stm32f4xx.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "usb_serial.h"

// Questao (b): teste de tempo de reacao com LED, botao USER por
// interrupcao externa e SysTick como base de tempo.

#define BUTTON_PIN   0U     // Botao USER da BlackPill: PA0
#define LED_PIN      13U    // LED onboard: PC13 (ativo em nivel baixo)
#define DEBOUNCE_MS  30U    // Tempo de nivel estavel para validar uma borda
#define WAIT_MIN_MS  2000U  // Espera minima antes do LED
#define WAIT_MAX_MS  5000U  // Espera maxima antes do LED
#define PAUSE_MS     2000U  // Pausa entre testes

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

// Gerador pseudoaleatorio xorshift32 para sortear a espera
static uint32_t random_state = 1U;

static uint32_t random_next(void)
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}

typedef enum
{
    ESTADO_PRONTO = 0,  // Anuncia o teste
    ESTADO_ARMANDO,     // Espera o botao solto para sortear a espera
    ESTADO_ESPERA,      // LED apagado; pressionar agora e "Muito cedo!"
    ESTADO_ESTIMULO,    // LED aceso; mede o tempo de reacao
    ESTADO_PAUSA        // Intervalo antes do proximo teste
} estado_t;

int main(void)
{
    // Configura clock, SysTick (1 ms) e USB CDC; espera o terminal abrir
    usb_serial_init();
    setvbuf(stdout, NULL, _IONBF, 0);

    led_init();
    button_interrupt_init();

    // Semente: instante em que o terminal abriu + SysTick + ID unico do chip
    random_state = millis() ^ (SysTick->VAL << 8) ^ *(volatile uint32_t *)UID_BASE;
    if (random_state == 0U)
    {
        random_state = 1U;
    }

    printf("Questao (b): teste de reacao.\r\n");

    estado_t estado = ESTADO_PRONTO;
    uint32_t inicio = 0U;     // Inicio da espera ou da pausa
    uint32_t espera_ms = 0U;  // Espera sorteada antes do LED
    uint32_t estimulo = 0U;   // Instante em que o LED acendeu

    while (1)
    {
        usb_serial_task();

        uint32_t at;
        bool pressionou = button_update(&at) && button_pressed;

        switch (estado)
        {
            case ESTADO_PRONTO:
                printf("Teste pronto. Solte USER e aguarde o LED acender.\r\n");
                estado = ESTADO_ARMANDO;
                break;

            case ESTADO_ARMANDO:
                // So comeca com o botao solto e sem borda em validacao
                if (!button_pressed && edge_pending == 0U)
                {
                    espera_ms = WAIT_MIN_MS + random_next() % (WAIT_MAX_MS - WAIT_MIN_MS + 1U);
                    inicio = millis();
                    estado = ESTADO_ESPERA;
                }
                break;

            case ESTADO_ESPERA:
                if (pressionou)
                {
                    printf("Muito cedo!\r\n");
                    inicio = millis();
                    estado = ESTADO_PAUSA;
                }
                else if ((uint32_t)(millis() - inicio) >= espera_ms &&
                         !button_pressed && edge_pending == 0U)
                {
                    // Nao acende com uma pressao ainda em validacao
                    led_set(true);
                    estimulo = millis();
                    printf("LED aceso! Pressione USER.\r\n");
                    estado = ESTADO_ESTIMULO;
                }
                break;

            case ESTADO_ESTIMULO:
                if (pressionou)
                {
                    // Instante da borda (EXTI) menos o instante do estimulo
                    led_set(false);
                    printf("Tempo de reacao: %lu ms\r\n", (unsigned long)(at - estimulo));
                    inicio = millis();
                    estado = ESTADO_PAUSA;
                }
                break;

            case ESTADO_PAUSA:
            default:
                if ((uint32_t)(millis() - inicio) >= PAUSE_MS)
                {
                    estado = ESTADO_PRONTO;
                }
                break;
        }
    }
}
