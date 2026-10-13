#ifndef TEST_COMPILER_COMPAT_H
#define TEST_COMPILER_COMPAT_H

/* TCC 0.9.27 nao implementa _Static_assert. Nos testes, preserva a verificacao
 * de tamanho usada dentro de button_report_poll; o ARM GCC usa C11 nativo. */
#ifdef __TINYC__
#define _Static_assert(condition, message) ((void)sizeof(char[(condition) ? 1 : -1]))
#endif

#endif
