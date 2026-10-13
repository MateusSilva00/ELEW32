#include "clock.h"
#include "stm32f4xx.h"

#if HSE_VALUE != 25000000U
#error "Os divisores desta configuracao exigem um cristal HSE de 25 MHz."
#endif

/* Limite de leituras, nao de milissegundos: SysTick ainda esta desligado. */
#define CLOCK_WAIT_LIMIT 1000000U

static bool wait_bits(volatile uint32_t *reg, uint32_t mask, uint32_t expected)
{
    for (uint32_t attempt = 0; attempt < CLOCK_WAIT_LIMIT; ++attempt) {
        if ((*reg & mask) == expected) {
            return true;
        }
    }
    return false;
}

bool clock_init_96mhz(void)
{
    /* Mantem a CPU em HSI enquanto configura o PLL. */
    RCC->CR |= RCC_CR_HSION;
    if (!wait_bits(&RCC->CR, RCC_CR_HSIRDY, RCC_CR_HSIRDY)) {
        return false;
    }
    RCC->CFGR &= ~RCC_CFGR_SW;
    if (!wait_bits(&RCC->CFGR, RCC_CFGR_SWS, RCC_CFGR_SWS_HSI)) {
        return false;
    }
    RCC->CR &= ~RCC_CR_PLLON;
    if (!wait_bits(&RCC->CR, RCC_CR_PLLRDY, 0U)) {
        return false;
    }

    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    (void)RCC->APB1ENR;
    /* Escala 1: permite ate 100 MHz. VOS e configurado com PLL desligado. */
    PWR->CR = (PWR->CR & ~PWR_CR_VOS) | PWR_CR_VOS;

    /* Cristal externo, sem bypass. A funcao pressupoe inicializacao apos reset. */
    RCC->CR &= ~RCC_CR_HSEBYP;
    RCC->CR |= RCC_CR_HSEON;
    if (!wait_bits(&RCC->CR, RCC_CR_HSERDY, RCC_CR_HSERDY)) {
        return false;
    }

    /* 25 MHz / M=25 * N=192 = VCO de 192 MHz.
     * P=2 -> SYSCLK 96 MHz; Q=4 -> clock USB de 48 MHz.
     * PLLP codificado como zero significa divisor 2. */
    RCC->PLLCFGR = (RCC->PLLCFGR & ~(RCC_PLLCFGR_PLLM | RCC_PLLCFGR_PLLN |
                        RCC_PLLCFGR_PLLP | RCC_PLLCFGR_PLLQ | RCC_PLLCFGR_PLLSRC)) |
                  (25U << RCC_PLLCFGR_PLLM_Pos) |
                  (192U << RCC_PLLCFGR_PLLN_Pos) |
                  (4U << RCC_PLLCFGR_PLLQ_Pos) | RCC_PLLCFGR_PLLSRC_HSE;

    /* A placa opera em 3,3 V: 3 wait states antes de aumentar HCLK para 96 MHz. */
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) | FLASH_ACR_LATENCY_3WS |
                 FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN;
    if (!wait_bits(&FLASH->ACR, FLASH_ACR_LATENCY, FLASH_ACR_LATENCY_3WS)) {
        return false;
    }

    /* HCLK=96 MHz, APB1=48 MHz (limite 50), APB2=96 MHz (limite 100). */
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)) |
                RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV1;
    RCC->CR |= RCC_CR_PLLON;
    if (!wait_bits(&RCC->CR, RCC_CR_PLLRDY, RCC_CR_PLLRDY) ||
        !wait_bits(&PWR->CSR, PWR_CSR_VOSRDY, PWR_CSR_VOSRDY)) {
        return false;
    }

    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    if (!wait_bits(&RCC->CFGR, RCC_CFGR_SWS, RCC_CFGR_SWS_PLL)) {
        return false;
    }
    SystemCoreClockUpdate(); /* Consulta os registradores; nao mede o cristal. */
    return SystemCoreClock == 96000000U;
}
