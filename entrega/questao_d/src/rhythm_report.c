#include "rhythm_report.h"
#include "rhythm_board.h"
#include "timebase.h"
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

static const char *name(rhythm_class_t value)
{
    return value == RHYTHM_LONG ? "longo" : "curto";
}

/* Maior linha: anuncio de 12 trechos ou intervalo final com o resultado;
 * os testes C conferem que ambas cabem em USB_SERIAL_TX_CAPACITY. */
static char *announce(char *out)
{
    rhythm_sequence_t sequence = rhythm_board_sequence();
    out = text(out, "Nivel ");
    out = number(out, sequence.steps - RHYTHM_INITIAL_STEPS + 1U);
    out = text(out, " (");
    out = number(out, sequence.steps);
    out = text(out, " trechos):");
    for (unsigned int index = 0U; index < sequence.steps; ++index) {
        *out++ = ' ';
        out = text(out, name(sequence.pattern[index]));
    }
    return text(out, "\r\n");
}

static char *hint(char *out)
{
    out = text(out, "Observe o LED; depois repita com ");
    out = number(out, rhythm_board_sequence().steps + 1U);
    return text(out, " cliques.\r\n");
}

static char *event_line(char *out, const rhythm_event_t *event)
{
    if (event->outcome == RHYTHM_TIMEOUT) {
        out = text(out, "Sem clique por 2 s: ");
        out = number(out, event->received);
        *out++ = '/';
        out = number(out, event->steps);
        out = text(out, " intervalos.\r\n");
    } else {
        out = text(out, "Intervalo ");
        out = number(out, event->received);
        *out++ = '/';
        out = number(out, event->steps);
        out = text(out, ": ");
        out = number(out, event->interval_ms);
        out = text(out, " ms -> ");
        out = text(out, name(event->measured));
        if (event->measured == event->expected) {
            out = text(out, " (ok)\r\n");
        } else {
            out = text(out, " (esperado ");
            out = text(out, name(event->expected));
            out = text(out, ")\r\n");
        }
    }
    if (event->outcome == RHYTHM_CORRECT) {
        out = text(out, event->steps < RHYTHM_MAX_STEPS ?
                   "Tentativa correta! Proximo nivel com mais um trecho.\r\n" :
                   "Tentativa correta! Nivel maximo; nova sequencia de 3.\r\n");
    } else if (event->outcome != RHYTHM_PENDING) {
        out = text(out, "Tentativa incorreta. Repita a mesma sequencia.\r\n");
    }
    return out;
}

void rhythm_report_poll(void)
{
    enum { ANNOUNCE, ANNOUNCE_TX, HINT, HINT_TX, RUN, EVENT_TX, PAUSE };
    static unsigned int phase;
    static uint32_t session;
    static uint32_t pause_at;
    static bool final_in_flight;
    uint32_t current = usb_serial_session();
    usb_serial_tx_result_t tx = usb_serial_take_result();
    if (current != session) {
        session = current;
        rhythm_board_session(current != 0U);
        phase = ANNOUNCE;
        return; /* Conclusao antiga nao avanca a nova sessao. */
    }
    if (session == 0U) { return; }
    if (phase == ANNOUNCE_TX || phase == HINT_TX || phase == EVENT_TX) {
        if (tx != USB_SERIAL_TX_DONE && tx != USB_SERIAL_TX_ABORTED) { return; }
        if (tx == USB_SERIAL_TX_ABORTED) {
            phase = phase == ANNOUNCE_TX ? ANNOUNCE : phase == HINT_TX ? HINT : RUN;
        } else if (phase == ANNOUNCE_TX) {
            phase = HINT;
        } else if (phase == HINT_TX) {
            /* A demonstracao so comeca depois de a instrucao ser transmitida. */
            (void)rhythm_board_start();
            phase = RUN;
        } else {
            rhythm_board_pop();
            if (final_in_flight) {
                pause_at = millis();
                phase = PAUSE;
            } else {
                phase = RUN;
            }
        }
        return;
    }
    if (phase == PAUSE) {
        if ((uint32_t)(millis() - pause_at) >= RHYTHM_PAUSE_MS) {
            (void)rhythm_board_next();
            phase = ANNOUNCE;
        }
        return;
    }

    char line[USB_SERIAL_TX_CAPACITY];
    char *end;
    rhythm_event_t event;
    unsigned int next = phase + 1U;
    if (phase == ANNOUNCE) {
        end = announce(line);
    } else if (phase == HINT) {
        end = hint(line);
    } else if (rhythm_board_peek(&event)) {
        end = event_line(line, &event);
        final_in_flight = event.outcome != RHYTHM_PENDING;
        next = EVENT_TX;
    } else {
        return;
    }
    if (usb_serial_write(line, (uint16_t)(end - line))) { phase = next; }
}
