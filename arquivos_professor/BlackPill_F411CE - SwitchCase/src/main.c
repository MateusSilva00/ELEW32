#include "serial.h"

int main(void)
{
    serial_init(115200);

    serial_write("\r\nSerial OK\r\n");
    serial_write("Digite 1, 2 ou 3:\r\n");

    while (1)
    {
        char c = serial_read_char();

        if (c == '\r' || c == '\n')
            continue;

        switch (c)
        {
            case '1':
                serial_write("Estado 1\r\n");
                break;

            case '2':
                serial_write("Estado 2\r\n");
                break;

            case '3':
                serial_write("Estado 3\r\n");
                break;

            default:
                serial_write("Comando invalido\r\n");
                break;
        }
    }
}
