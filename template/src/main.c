#include "stm32f4xx.h"
#include "clock.h"
#include "timebase.h"
#include "usb_serial.h"

/* Comeca em zero: o startup limpa a secao .bss antes de chamar main. */
volatile uint32_t firmware_iniciado;

int main(void)
{
    /* LED USER em PC13, ativo em nivel baixo. Primeiro mantemos o LED apagado. */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    (void)RCC->AHB1ENR;
    GPIOC->BSRR = GPIO_BSRR_BS13;
    GPIOC->OTYPER &= ~(1U << 13);
    GPIOC->OSPEEDR &= ~(3U << (13 * 2));
    GPIOC->PUPDR &= ~(3U << (13 * 2));
    GPIOC->MODER = (GPIOC->MODER & ~(3U << (13 * 2))) | (1U << (13 * 2));

    /* Confere .bss e .data antes de modificar suas variaveis. */
    if ((firmware_iniciado != 0U) || (SystemCoreClock != 16000000U)) {
        while (1) {
            __NOP(); /* Falha de inicializacao: LED permanece apagado. */
        }
    }

    if (!clock_init_96mhz() || !timebase_init()) {
        while (1) {
            __NOP(); /* Falha de clock/base de tempo: LED permanece apagado. */
        }
    }
    if (!usb_serial_init()) {
        while (1) {
            __NOP(); /* USB nao inicializou: LED permanece apagado. */
        }
    }
    firmware_iniciado = 1U;
    GPIOC->BSRR = GPIO_BSRR_BR13;
    uint32_t last_toggle = millis();

    while (1) {
        usb_serial_poll();
        uint32_t now = millis();
        /* Subtracao unsigned continua correta quando o contador volta a zero. */
        if ((uint32_t)(now - last_toggle) >= 500U) {
            last_toggle += 500U;
            GPIOC->ODR ^= (1U << 13);
        }
        __WFI(); /* Aguarda uma interrupcao; SysTick acorda a CPU a cada 1 ms. */
    }
}
