#ifndef APP_USB_SERIAL_H
#define APP_USB_SERIAL_H

#include <stdbool.h>
#include <stdint.h>

#define USB_SERIAL_TX_CAPACITY 128U

typedef enum {
    USB_SERIAL_TX_NONE,
    USB_SERIAL_TX_BUSY,
    USB_SERIAL_TX_DONE,
    USB_SERIAL_TX_ABORTED
} usb_serial_tx_result_t;

/* Inicializar depois do clock e da base de tempo. As APIs sao para o main. */
bool usb_serial_init(void);
void usb_serial_poll(void);
/* Copia os dados para buffer proprio; false = ocupado/sem terminal/tamanho invalido.
 * true indica transferencia aceita, nao confirma leitura pelo terminal.
 * Consumir DONE/ABORTED com take_result antes de iniciar outro write. */
bool usb_serial_write(const void *data, uint16_t length);
/* Consulta/consome resultado do ultimo write da aplicacao (nao da saudacao).
 * DONE confirma o transporte USB, nao leitura pelo aplicativo do computador. */
usb_serial_tx_result_t usb_serial_take_result(void);

#endif
