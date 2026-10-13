#include "stm32f4xx.h"

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

    /* O reset usa HSI de 16 MHz. Esta chamada apenas atualiza a variavel CMSIS. */
    SystemCoreClockUpdate();
    firmware_iniciado = 1U;
    GPIOC->BSRR = GPIO_BSRR_BR13; /* LED aceso: startup e main executados. */

    while (1) {
        /* A base de tempo e as interrupcoes serao adicionadas depois. */
        __NOP();
    }
}
