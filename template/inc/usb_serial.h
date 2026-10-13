#ifndef APP_USB_SERIAL_H
#define APP_USB_SERIAL_H

#include <stdbool.h>
#include <stdint.h>

#define USB_SERIAL_TX_CAPACITY 128U

/* Inicializar depois do clock e da base de tempo. As APIs sao para o main. */
bool usb_serial_init(void);
void usb_serial_poll(void);
/* Copia os dados para buffer proprio; false = ocupado/sem terminal/tamanho invalido.
 * true indica transferencia aceita, nao confirma leitura pelo terminal. */
bool usb_serial_write(const void *data, uint16_t length);

#endif
