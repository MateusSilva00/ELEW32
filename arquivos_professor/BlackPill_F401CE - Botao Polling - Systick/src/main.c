#include "stm32f4xx.h"

#define LED_PORT    GPIOC
#define LED_PIN     13

#define BTN_PORT    GPIOA
#define BTN_PIN     0

#define DEBOUNCE_MS 20U


volatile uint32_t ms_ticks = 0;


// Configura pino como saída
void gpio_out(GPIO_TypeDef* port, uint32_t pin)
{
    port->MODER &= ~(3U << (pin * 2U));
    port->MODER |=  (1U << (pin * 2U));

    port->OTYPER &= ~(1U << pin);        // push-pull
    port->OSPEEDR |= (3U << (pin * 2U)); // alta velocidade
    port->PUPDR &= ~(3U << (pin * 2U));  // sem pull
}


// Configura pino como entrada com pull-up
void gpio_in_pullup(GPIO_TypeDef* port, uint32_t pin)
{
    port->MODER &= ~(3U << (pin * 2U));  // entrada

    port->PUPDR &= ~(3U << (pin * 2U));
    port->PUPDR |=  (1U << (pin * 2U));  // pull-up
}


// Coloca pino em nível alto
void pin_set(GPIO_TypeDef* port, uint32_t pin)
{
    port->BSRR = (1U << pin);
}


// Coloca pino em nível baixo
void pin_clr(GPIO_TypeDef* port, uint32_t pin)
{
    port->BSRR = (1U << (pin + 16U));
}


// LED da BlackPill é ativo em nível baixo
#define LED_ON()   pin_clr(LED_PORT, LED_PIN)
#define LED_OFF()  pin_set(LED_PORT, LED_PIN)


// Interrupção do SysTick
// Executa a cada 1 ms
void SysTick_Handler(void)
{
    ms_ticks++;
}


// Delay em milissegundos usando SysTick
void delay_ms(uint32_t ms)
{
    uint32_t start = ms_ticks;

    while ((ms_ticks - start) < ms)
    {
        // espera
    }
}


int main(void)
{
    // Habilita clock dos GPIOs A e C
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN |
                    RCC_AHB1ENR_GPIOCEN;

    // Pequena leitura para garantir que o clock foi habilitado
    (void)RCC->AHB1ENR;

    // Configura LED e botão
    gpio_out(LED_PORT, LED_PIN);
    gpio_in_pullup(BTN_PORT, BTN_PIN);

    // LED começa apagado
    LED_OFF();

    // Atualiza valor de SystemCoreClock
    SystemCoreClockUpdate();

    // Configura SysTick para gerar interrupção a cada 1 ms
    SysTick_Config(SystemCoreClock / 1000U);


    while (1)
    {

        if ((BTN_PORT->IDR & (1U << BTN_PIN)) == 0U)
        {
            // Debounce de 20 ms
            delay_ms(DEBOUNCE_MS);

            // Confirma se o botão continua pressionado
            if ((BTN_PORT->IDR & (1U << BTN_PIN)) == 0U)
            {
                LED_ON();

                // Espera o botão ser solto
                while ((BTN_PORT->IDR & (1U << BTN_PIN)) == 0U)
                {
                 
                }
            }
        }
        else
        {
            LED_OFF();
        }
    }
}