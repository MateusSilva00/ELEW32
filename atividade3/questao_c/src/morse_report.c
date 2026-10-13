#include "morse_report.h"
#include "morse_board.h"
#include "usb_serial.h"

static char *text(char *out, const char *value)
{
    while (*value != '\0') { *out++ = *value++; }
    return out;
}

static char *number(char *out, uint32_t value)
{
    char digits[10];
    uint32_t count = 0U;
    do {
        digits[count++] = (char)('0' + value % 10U);
        value /= 10U;
    } while (value != 0U);
    while (count != 0U) { *out++ = digits[--count]; }
    return out;
}

void morse_report_poll(void)
{
    enum { IDLE, EVENT, LOSS };
    static unsigned int in_flight;
    static uint32_t session;
    static uint32_t lost_reported;
    static uint32_t lost_in_flight;
    uint32_t current = usb_serial_session();
    usb_serial_tx_result_t tx = usb_serial_take_result();
    if (current != session) {
        session = current;
        morse_board_session(current != 0U);
        in_flight = IDLE;
        lost_reported = 0U;
        lost_in_flight = 0U;
        return; /* Conclusao antiga nao remove evento da nova sessao. */
    }
    if (session == 0U) { return; }
    if (in_flight != IDLE) {
        if (tx != USB_SERIAL_TX_DONE && tx != USB_SERIAL_TX_ABORTED) { return; }
        if (tx == USB_SERIAL_TX_DONE) {
            if (in_flight == EVENT) { morse_board_pop(); }
            else { lost_reported += lost_in_flight; }
        }
        in_flight = IDLE; /* ABORTED preserva evento/perdas para repetir. */
    }

    morse_result_t result;
    /* Maior linha: prefixo fixo + 4 simbolos + diagnostico, menor que 96.
     * usb_serial_write copia a linha para o buffer persistente de 128 bytes. */
    char line[96];
    char *end;
    if (morse_board_peek(&result)) {
        end = text(line, result.kind == MORSE_TOO_LONG ? "Morse (prefixo): " : "Morse ");
        end = text(end, result.symbols);
        end = text(end, " -> ");
        if (result.kind == MORSE_LETTER) { *end++ = result.letter; }
        else if (result.kind == MORSE_UNKNOWN) { end = text(end, "sequencia invalida"); }
        else { end = text(end, "mais de 4 simbolos"); }
        end = text(end, "\r\n");
        if (usb_serial_write(line, (uint16_t)(end - line))) { in_flight = EVENT; }
        return;
    }
    uint32_t lost = morse_board_lost() - lost_reported;
    if (lost != 0U) {
        end = text(line, "Resultados perdidos: ");
        end = number(end, lost);
        end = text(end, " (fila cheia)\r\n");
        if (usb_serial_write(line, (uint16_t)(end - line))) {
            lost_in_flight = lost;
            in_flight = LOSS;
        }
    }
}
