#include <stdint.h>

// Protótipo da função implementada em Assembly no arquivo src/soma.s
extern int32_t soma_assembly(int32_t a, int32_t b);

void delay(volatile uint32_t count) {
    while (count--) {
        __asm__("nop");
    }
}

int main(void) {
    volatile int32_t x = 15;
    volatile int32_t y = 25;
    volatile int32_t resultado = 0;

    while (1) {
        // Chamada da função em Assembly:
        // x vai para R0, y vai para R1
        resultado = soma_assembly(x, y);

        x++;
        delay(50000);
    }

    return 0;
}
