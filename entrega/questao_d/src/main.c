#include "stm32f4xx.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "usb_serial.h"

// Questao (d): sequencia ritmica. O LED demonstra a sequencia, o usuario
// a repete com USER (EXTI) e o SysTick mede os intervalos entre cliques.

#define BUTTON_PIN   0U     // Botao USER da BlackPill: PA0
#define LED_PIN      13U    // LED onboard: PC13 (ativo em nivel baixo)
#define DEBOUNCE_MS  30U    // Tempo de nivel estavel para validar uma borda
#define SHORT_MS       300U   // Trecho curto demonstrado no LED
#define LONG_MS        900U   // Trecho longo demonstrado no LED
#define LONG_MIN_MS    600U   // Intervalo medido: curto < 600 ms <= longo
#define TIMEOUT_MS     2000U  // Sem clique por 2 s encerra a tentativa
#define FLASH_MS       100U   // Duracao de cada batida do LED
#define LEAD_MS        1000U  // Pausa antes da primeira batida
#define PAUSE_MS       1500U  // Pausa entre rodadas
#define INITIAL_STEPS  3U
#define MAX_STEPS      12U

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

// Gerador pseudoaleatorio xorshift32 para sortear os trechos
static uint32_t random_state = 1U;

static uint32_t random_next(void)
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}

// Sequencia atual: true = trecho longo, false = trecho curto
static bool sequencia[MAX_STEPS];
static unsigned int trechos = 0U;

static bool trecho_aleatorio(void)
{
    return (random_next() & 0x80000000U) != 0U;
}

// Nova sequencia de 3 trechos, sempre com curtos e longos
static void nova_sequencia(void)
{
    trechos = INITIAL_STEPS;
    for (unsigned int i = 0U; i < INITIAL_STEPS; i++)
    {
        sequencia[i] = trecho_aleatorio();
    }
    if (sequencia[0] == sequencia[1] && sequencia[1] == sequencia[2])
    {
        sequencia[1] = !sequencia[1];
    }
}

static const char *nome(bool longo)
{
    return longo ? "longo" : "curto";
}

static uint32_t duracao_nominal(bool longo)
{
    return longo ? LONG_MS : SHORT_MS;
}

// LED da demonstracao: uma batida de FLASH_MS em cada clique da sequencia.
// Calculado a partir do tempo, sem bloquear o laco principal.
static bool demo_led(uint32_t decorrido)
{
    uint32_t batida = 0U;

    for (unsigned int i = 0U; i <= trechos; i++)
    {
        if (decorrido >= batida && (decorrido - batida) < FLASH_MS)
        {
            return true;
        }
        if (i < trechos)
        {
            batida += duracao_nominal(sequencia[i]);
        }
    }
    return false;
}

static uint32_t demo_duracao(void)
{
    uint32_t total = FLASH_MS;

    for (unsigned int i = 0U; i < trechos; i++)
    {
        total += duracao_nominal(sequencia[i]);
    }
    return total;
}

typedef enum
{
    ESTADO_ANUNCIO = 0,  // Mostra a sequencia na serial
    ESTADO_DEMO,         // LED toca a sequencia; cliques ignorados
    ESTADO_ESPERA,       // Aguarda o primeiro clique (sem prazo)
    ESTADO_ENTRADA,      // Mede os intervalos entre cliques
    ESTADO_PAUSA         // Intervalo antes da proxima rodada
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
    nova_sequencia();

    printf("Questao (d): sequencia ritmica.\r\n");
    printf("Curto <%u ms; longo >=%u ms; 2 s sem clique encerra a tentativa.\r\n",
           LONG_MIN_MS, LONG_MIN_MS);

    estado_t estado = ESTADO_ANUNCIO;
    uint32_t inicio = 0U;          // Inicio da demonstracao ou da pausa
    uint32_t ultimo_clique = 0U;
    unsigned int recebidos = 0U;   // Intervalos medidos na tentativa
    bool errou = false;

    while (1)
    {
        usb_serial_task();

        uint32_t at = 0U;
        bool mudou = button_update(&at);
        bool clicou = mudou && button_pressed;

        // Fora da demonstracao, o LED acompanha o botao
        if (mudou && estado != ESTADO_DEMO)
        {
            led_set(button_pressed);
        }

        switch (estado)
        {
            case ESTADO_ANUNCIO:
                printf("Nivel %u (%u trechos):", trechos - INITIAL_STEPS + 1U, trechos);
                for (unsigned int i = 0U; i < trechos; i++)
                {
                    printf(" %s", nome(sequencia[i]));
                }
                printf("\r\nObserve o LED; depois repita com %u cliques.\r\n", trechos + 1U);
                inicio = millis() + LEAD_MS;
                estado = ESTADO_DEMO;
                break;

            case ESTADO_DEMO:
            {
                uint32_t decorrido = millis() - inicio;

                if ((int32_t)decorrido < 0)
                {
                    led_set(false);  // Ainda antes da primeira batida
                }
                else if (decorrido < demo_duracao())
                {
                    led_set(demo_led(decorrido));
                }
                else
                {
                    led_set(button_pressed);
                    estado = ESTADO_ESPERA;
                }
                break;
            }

            case ESTADO_ESPERA:
                if (clicou)
                {
                    // O primeiro clique so marca o inicio
                    ultimo_clique = at;
                    recebidos = 0U;
                    errou = false;
                    estado = ESTADO_ENTRADA;
                }
                break;

            case ESTADO_ENTRADA:
            {
                bool terminou = false;
                uint32_t intervalo = at - ultimo_clique;

                if ((clicou && intervalo >= TIMEOUT_MS) ||
                    (!clicou && edge_pending == 0U &&
                     (uint32_t)(millis() - ultimo_clique) >= TIMEOUT_MS))
                {
                    // Prazo vencido: o clique atrasado nao conta como intervalo
                    printf("Sem clique por 2 s: %u/%u intervalos.\r\n", recebidos, trechos);
                    errou = true;
                    terminou = true;
                }
                else if (clicou)
                {
                    bool medido = intervalo >= LONG_MIN_MS;
                    bool esperado = sequencia[recebidos];

                    recebidos++;
                    ultimo_clique = at;

                    if (medido == esperado)
                    {
                        printf("Intervalo %u/%u: %lu ms -> %s (ok)\r\n",
                               recebidos, trechos, (unsigned long)intervalo, nome(medido));
                    }
                    else
                    {
                        errou = true;
                        printf("Intervalo %u/%u: %lu ms -> %s (esperado %s)\r\n",
                               recebidos, trechos, (unsigned long)intervalo,
                               nome(medido), nome(esperado));
                    }

                    // A tentativa termina no ultimo intervalo, mostrando todos os erros
                    terminou = (recebidos == trechos);
                }

                if (terminou)
                {
                    if (errou)
                    {
                        printf("Tentativa incorreta. Repita a mesma sequencia.\r\n");
                    }
                    else if (trechos < MAX_STEPS)
                    {
                        printf("Tentativa correta! Proximo nivel com mais um trecho.\r\n");
                        sequencia[trechos++] = trecho_aleatorio();
                    }
                    else
                    {
                        printf("Tentativa correta! Nivel maximo; nova sequencia de 3.\r\n");
                        nova_sequencia();
                    }
                    inicio = millis();
                    estado = ESTADO_PAUSA;
                }
                break;
            }

            case ESTADO_PAUSA:
            default:
                if ((uint32_t)(millis() - inicio) >= PAUSE_MS)
                {
                    estado = ESTADO_ANUNCIO;
                }
                break;
        }
    }
}
