#ifndef USB_SERIAL_H
#define USB_SERIAL_H

#include <stdint.h>

void usb_serial_init(void);
int usb_serial_getchar(void);

// Acrescentado para a Atividade 3:
// tempo em ms contado pelo SysTick e tarefa USB para o laco principal.
uint32_t millis(void);
void usb_serial_task(void);

#endif
