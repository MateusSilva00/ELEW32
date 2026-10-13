#include <stdbool.h>
#include <stdio.h>

#include "usb_serial.h"

typedef enum
{
    ESTADO_AGUARDANDO = 0,
    ESTADO_PAR,
    ESTADO_IMPAR,
    ESTADO_ERRO
} estado_t;

int main(void)
{
    usb_serial_init();

    printf("\r\nFSM PAR OU IMPAR\r\n");
    printf("Digite um inteiro decimal nao negativo e pressione Enter.\r\n> ");

    estado_t estado = ESTADO_AGUARDANDO;
    bool ignorar_lf = false;

    while (1)
    {
        int ch = usb_serial_getchar();

        if (ch < 0)
        {
            continue;
        }

        // Ignora o LF depois de CR
        if (ignorar_lf && ch == '\n')
        {
            ignorar_lf = false;
            continue;
        }

        ignorar_lf = false;

        // Enter encerra a entrada
        if (ch == '\r' || ch == '\n')
        {
            if (ch == '\r')
            {
                ignorar_lf = true;
            }

            switch (estado)
            {
                case ESTADO_PAR:
                    printf("\r\nPAR\r\n");
                    break;

                case ESTADO_IMPAR:
                    printf("\r\nIMPAR\r\n");
                    break;

                case ESTADO_ERRO:
                    printf("\r\nERRO: entrada invalida.\r\n");
                    break;

                case ESTADO_AGUARDANDO:
                default:
                    printf("\r\nERRO: entrada vazia.\r\n");
                    break;
            }

            estado = ESTADO_AGUARDANDO;
            printf("> ");
            continue;
        }

        switch (estado)
        {
            case ESTADO_AGUARDANDO:
            case ESTADO_PAR:
            case ESTADO_IMPAR:
                if (ch >= '0' && ch <= '9')
                {
                    if (((ch - '0') % 2) == 0)
                    {
                        estado = ESTADO_PAR;
                    }
                    else
                    {
                        estado = ESTADO_IMPAR;
                    }
                }
                else
                {
                    estado = ESTADO_ERRO;
                }
                break;

            case ESTADO_ERRO:
                // Fica em erro ate Enter
                break;
        }
    }
}
