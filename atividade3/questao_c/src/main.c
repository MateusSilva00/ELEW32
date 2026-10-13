#include "stm32f4xx.h"
#include "clock.h"
#include "timebase.h"
#include "usb_serial.h"

/* O startup limpa .bss antes de chamar main. */
volatile uint32_t firmware_iniciado;

int main(void)
{
    /* PC13 e ativo em nivel baixo. Prepara a saida sem acender o LED.
     * Nesta base da questao (c), o LED permanece apagado. */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    (void)RCC->AHB1ENR;
    GPIOC->BSRR = GPIO_BSRR_BS13;
    GPIOC->OTYPER &= ~(1U << 13);
    GPIOC->OSPEEDR &= ~(3U << (13 * 2));
    GPIOC->PUPDR &= ~(3U << (13 * 2));
    GPIOC->MODER = (GPIOC->MODER & ~(3U << (13 * 2))) | (1U << (13 * 2));

    if ((firmware_iniciado != 0U) || (SystemCoreClock != 16000000U)) {
        while (1) {
            __NOP();
        }
    }
    if (!clock_init_96mhz() || !timebase_init() || !usb_serial_init()) {
        while (1) {
            __NOP(); /* Falha de inicializacao: nao anuncia prontidao. */
        }
    }
    firmware_iniciado = 1U;

    while (1) {
        /* Toda formatacao/transmissao ocorre no main, sem bloquear ISRs. */
        usb_serial_poll();
        __WFI();
    }
}
