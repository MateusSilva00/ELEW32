#include "button_report.h"
#include "button.h"
#include "usb_serial.h"

/* Formatacao pequena, sem printf/heap e sem executar dentro da interrupcao. */
static char *append_text(char *out, const char *text)
{
    while (*text != '\0') {
        *out++ = *text++;
    }
    return out;
}

static char *append_u32(char *out, uint32_t value)
{
    char digits[10];
    uint32_t count = 0U;
    do {
        digits[count++] = (char)('0' + value % 10U);
        value /= 10U;
    } while (value != 0U);
    while (count != 0U) {
        *out++ = digits[--count];
    }
    return out;
}

void button_report_poll(void)
{
    enum { IDLE, EVENT, LOSS };
    static unsigned int in_flight;
    static uint32_t lost_reported;
    static uint32_t lost_in_flight;
    if (in_flight != IDLE) {
        usb_serial_tx_result_t result = usb_serial_take_result();
        if (result == USB_SERIAL_TX_BUSY || result == USB_SERIAL_TX_NONE) {
            return;
        }
        if (result == USB_SERIAL_TX_DONE) {
            if (in_flight == EVENT) {
                button_event_pop();
            } else {
                lost_reported += lost_in_flight;
            }
        }
        /* Se abortou, a fila/quantidade de perdas continua pendente. */
        in_flight = IDLE;
    }

    button_event_t event;
    char line[96];
    _Static_assert(sizeof(line) <= USB_SERIAL_TX_CAPACITY, "Linha excede buffer USB");
    char *end;
    if (button_event_peek(&event)) {
        end = append_text(line, "Botao ");
        end = append_u32(end, event.number);
        end = append_text(end, " pressionado em ");
        end = append_u32(end, event.timestamp_ms);
        end = append_text(end, " ms\r\n");
        if (usb_serial_write(line, (uint16_t)(end - line))) {
            in_flight = EVENT;
        }
        return;
    }
    uint32_t lost = button_lost_total() - lost_reported;
    if (lost != 0U) {
        end = append_text(line, "Eventos perdidos: ");
        end = append_u32(end, lost);
        end = append_text(end, " (fila cheia)\r\n");
        if (usb_serial_write(line, (uint16_t)(end - line))) {
            lost_in_flight = lost;
            in_flight = LOSS;
        }
    }
}
