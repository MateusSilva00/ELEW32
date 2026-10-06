.syntax unified
.cpu cortex-m4
.thumb

.global soma_assembly
.type soma_assembly, %function

/*
 * Função ARM Assembly (Thumb-2):
 * int32_t soma_assembly(int32_t a, int32_t b)
 *
 * Padrão ARM AAPCS:
 *   - Parâmetro 'a' chega em R0
 *   - Parâmetro 'b' chega em R1
 *   - O resultado da função DEVE ser deixado em R0
 */
soma_assembly:
    ADD R0, R0, R1      @ R0 = R0 + R1 (soma os registradores)
    BX  LR              @ Retorna para a função chamadora (Branch and Exchange LR)
