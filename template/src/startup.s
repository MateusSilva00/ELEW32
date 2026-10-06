.syntax unified
.cpu cortex-m4
.thumb

.global g_pfnVectors
.global Reset_Handler

/* Tabela de Vetores de Interrupção do Cortex-M */
.section .isr_vector,"a",%progbits
.type g_pfnVectors, %object
g_pfnVectors:
    .word _estack            @ 0: Ponteiro inicial de Pilha (Topo da RAM)
    .word Reset_Handler      @ 1: Vetor de Reset (Ponto de entrada)
    .word NMI_Handler        @ 2: NMI Handler
    .word HardFault_Handler  @ 3: HardFault Handler

.text
.type Reset_Handler, %function
Reset_Handler:
    /* Chama a função main */
    BL main

    /* Loop infinito de segurança caso a main retorne */
Loop_Infinito:
    B Loop_Infinito

.thumb_func
NMI_Handler:
    B .

.thumb_func
HardFault_Handler:
    B .
