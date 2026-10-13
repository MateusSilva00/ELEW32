#include "stm32f4xx.h"
#include <stdint.h>

#define BUTTON_PIN   0U      // Botao USER: PA0
#define LED_PIN      13U     // LED onboard: PC13 (ativo em nivel baixo)
#define DEBOUNCE_MS  20U

static void delay_ms(uint32_t ms);

static void led_init(void)
{
    // Habilita o clock do GPIOC
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

    // PC13 como saida
    GPIOC->MODER &= ~(3U << (LED_PIN * 2U));
    GPIOC->MODER |=  (1U << (LED_PIN * 2U));

    // Push-pull e sem pull-up/pull-down
    GPIOC->OTYPER &= ~(1U << LED_PIN);
    GPIOC->PUPDR  &= ~(3U << (LED_PIN * 2U));

    // LED apagado inicialmente
    // O LED da BlackPill e ativo em nivel baixo
    GPIOC->BSRR = (1U << LED_PIN);
}

static void button_init(void)
{
    // Habilita o clock do GPIOA
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // PA0 como entrada
    GPIOA->MODER &= ~(3U << (BUTTON_PIN * 2U));

    // Pull-up interno
    // Solto:       PA0 = 1
    // Pressionado: PA0 = 0
    GPIOA->PUPDR &= ~(3U << (BUTTON_PIN * 2U));
    GPIOA->PUPDR |=  (1U << (BUTTON_PIN * 2U));
}

static uint8_t button_pressed(void)
{
    // Le diretamente o registrador de entrada do GPIOA
    if ((GPIOA->IDR & (1U << BUTTON_PIN)) == 0U)
    {
        return 1U;
    }

    return 0U;
}

static void led_toggle(void)
{
    GPIOC->ODR ^= (1U << LED_PIN);
}

static void wait_button_release_debounced(void)
{
    while (1)
    {
        // Espera detectar a liberacao
        while (button_pressed() != 0U)
        {
            // Continua lendo PA0 por polling
        }

        // Confirma se a liberacao permaneceu estavel
        delay_ms(DEBOUNCE_MS);

        if (button_pressed() == 0U)
        {
            return;
        }
    }
}

static void systick_polling_init(void)
{
    // Atualiza SystemCoreClock com a frequencia configurada no sistema
    SystemCoreClockUpdate();

    // Gera um evento COUNTFLAG a cada 1 ms.
    // IMPORTANTE: TICKINT nao e habilitado.
    // Portanto, o SysTick NAO gera interrupcao.
    SysTick->LOAD = (SystemCoreClock / 1000U) - 1U;
    SysTick->VAL  = 0U;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_ENABLE_Msk;
}

static void delay_ms(uint32_t ms)
{
    while (ms > 0U)
    {
        // Polling do COUNTFLAG do SysTick
        while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0U)
        {
            // Apenas espera o contador completar 1 ms
        }

        ms--;
    }
}

int main(void)
{
    led_init();
    button_init();
    systick_polling_init();

    while (1)
    {
        // POLLING: o processador consulta PA0 continuamente
        if (button_pressed() != 0U)
        {
            // Debounce do pressionamento
            delay_ms(DEBOUNCE_MS);

            // Confirma que o botao continua pressionado
            if (button_pressed() != 0U)
            {
                led_toggle();

                // Espera uma liberacao estavel antes de aceitar
                // um novo pressionamento
                wait_button_release_debounced();
            }
        }
    }
}
