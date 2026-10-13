#include "stm32f4xx.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "usb_serial.h"

// Questao (c): decodificador Morse. EXTI detecta pressao e soltura;
// o SysTick mede a duracao (ponto/traco) e o fim da letra.

#define BUTTON_PIN   0U     // Botao USER da BlackPill: PA0
#define LED_PIN      13U    // LED onboard: PC13 (ativo em nivel baixo)
#define DEBOUNCE_MS  30U    // Tempo de nivel estavel para validar uma borda
#define DASH_MIN_MS    300U   // Pressao menor: ponto; maior ou igual: traco
#define LETTER_GAP_MS  1000U  // Tempo solto que encerra a letra
#define MAX_SYMBOLS    4U     // A-Z tem no maximo 4 simbolos

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

// Tabela A-Z (ITU-R M.1677-1)
static const char *const morse_codes[26] =
{
    ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..",
    ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.",
    "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--.."
};

// Imprime a letra decodificada (ou o erro) e limpa a sequencia
static void morse_print(const char *simbolos, bool excesso)
{
    if (excesso)
    {
        printf("Morse (prefixo): %s -> mais de %u simbolos\r\n", simbolos, MAX_SYMBOLS);
        return;
    }

    for (unsigned int i = 0U; i < 26U; i++)
    {
        if (strcmp(simbolos, morse_codes[i]) == 0)
        {
            printf("Morse %s -> %c\r\n", simbolos, (char)('A' + i));
            return;
        }
    }

    printf("Morse %s -> sequencia invalida\r\n", simbolos);
}

int main(void)
{
    // Configura clock, SysTick (1 ms) e USB CDC; espera o terminal abrir
    usb_serial_init();
    setvbuf(stdout, NULL, _IONBF, 0);

    led_init();
    button_interrupt_init();

    printf("Questao (c): Morse A-Z.\r\n");
    printf("Ponto <%u ms; traco >=%u ms; intervalo entre letras: 1 s.\r\n",
           DASH_MIN_MS, DASH_MIN_MS);

    char simbolos[MAX_SYMBOLS + 1U] = {0};
    unsigned int quantidade = 0U;
    bool excesso = false;
    uint32_t pressionado_em = 0U;
    uint32_t solto_em = 0U;
    // Exige uma soltura antes de aceitar simbolos (boot com USER apertado)
    bool armado = !button_pressed;

    while (1)
    {
        usb_serial_task();

        uint32_t at;

        if (button_update(&at))
        {
            // LED aceso enquanto o botao esta pressionado
            led_set(button_pressed);

            if (button_pressed)
            {
                // Pressao iniciada 1 s ou mais apos a soltura: fecha a letra anterior
                if (quantidade > 0U && (uint32_t)(at - solto_em) >= LETTER_GAP_MS)
                {
                    morse_print(simbolos, excesso);
                    quantidade = 0U;
                    excesso = false;
                    simbolos[0] = '\0';
                }
                pressionado_em = at;
            }
            else if (!armado)
            {
                armado = true;
            }
            else
            {
                // Duracao medida entre as bordas de pressao e soltura
                uint32_t duracao = at - pressionado_em;
                char simbolo = (duracao < DASH_MIN_MS) ? '.' : '-';

                if (quantidade < MAX_SYMBOLS)
                {
                    simbolos[quantidade++] = simbolo;
                    simbolos[quantidade] = '\0';
                }
                else
                {
                    excesso = true;  // Mantem os 4 primeiros e sinaliza
                }
                solto_em = at;
            }
        }

        // Fim da letra: 1 s solto, sem nova borda em validacao
        if (quantidade > 0U && !button_pressed && edge_pending == 0U &&
            (uint32_t)(millis() - solto_em) >= LETTER_GAP_MS)
        {
            morse_print(simbolos, excesso);
            quantidade = 0U;
            excesso = false;
            simbolos[0] = '\0';
        }
    }
}
