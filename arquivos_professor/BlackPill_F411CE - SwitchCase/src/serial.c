#include "stm32f4xx.h"
#include "serial.h"

void serial_init(uint32_t baud)
{
    // USART1: PA9 = TX, PA10 = RX
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    GPIOA->MODER &= ~((3U << (9U * 2U)) | (3U << (10U * 2U)));
    GPIOA->MODER |=  ((2U << (9U * 2U)) | (2U << (10U * 2U)));

    GPIOA->AFR[1] &= ~((0xFU << 4U) | (0xFU << 8U));
    GPIOA->AFR[1] |=  ((7U << 4U) | (7U << 8U));

    USART1->CR1 = 0U;
    USART1->CR2 = 0U;
    USART1->CR3 = 0U;

    USART1->BRR = (SystemCoreClock + (baud / 2U)) / baud;

    USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

void serial_write(const char *text)
{
    while (*text != '\0')
    {
        while ((USART1->SR & USART_SR_TXE) == 0U) {}
        USART1->DR = (uint8_t)(*text++);
    }

    while ((USART1->SR & USART_SR_TC) == 0U) {}
}

char serial_read_char(void)
{
    while ((USART1->SR & USART_SR_RXNE) == 0U) {}
    return (char)(USART1->DR & 0xFFU);
}
