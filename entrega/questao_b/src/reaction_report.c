#include "reaction_report.h"
#include "reaction_board.h"
#include "usb_serial.h"
#include "timebase.h"
#include <string.h>

enum report_phase { READY, READY_TX, ARM, RUN, STIMULUS_TX, RESULT_TX, COOLDOWN };
static enum report_phase phase;
static uint32_t session;
static uint32_t cooldown_at;
static bool stimulus_reported;

static bool send_text(const char *text)
{
    return usb_serial_write(text, (uint16_t)strlen(text));
}

static bool send_result(uint32_t value)
{
    char line[64] = "Tempo de reacao: ";
    char digits[10];
    unsigned int count = 0U;
    char *end = line + strlen(line);
    do {
        digits[count++] = (char)('0' + value % 10U);
        value /= 10U;
    } while (value != 0U);
    while (count != 0U) { *end++ = digits[--count]; }
    memcpy(end, " ms\r\n", 5U);
    end += 5U;
    return usb_serial_write(line, (uint16_t)(end - line));
}

void reaction_report_poll(void)
{
    uint32_t current = usb_serial_session();
    usb_serial_tx_result_t tx = usb_serial_take_result();
    if (current != session) {
        session = current;
        if (current != 0U) { reaction_board_new_session(); }
        else { reaction_board_reset(); }
        phase = READY;
        stimulus_reported = false;
        /* Nao usar a conclusao de uma transferencia da sessao anterior.
         * Se ainda estiver BUSY, o transporte impede novos writes. */
        return;
    }
    if (session == 0U) { return; }

    if (phase == READY_TX || phase == STIMULUS_TX || phase == RESULT_TX) {
        if (tx != USB_SERIAL_TX_DONE && tx != USB_SERIAL_TX_ABORTED) { return; }
        if (phase == READY_TX) {
            phase = tx == USB_SERIAL_TX_DONE ? ARM : READY;
        } else if (phase == STIMULUS_TX) {
            stimulus_reported = tx == USB_SERIAL_TX_DONE;
            phase = RUN;
        } else if (tx == USB_SERIAL_TX_DONE) {
            reaction_board_reset();
            cooldown_at = millis();
            phase = COOLDOWN;
        } else {
            phase = RUN; /* Resultado permanece para repetir apos aborto. */
        }
        return;
    }
    if (phase == COOLDOWN) {
        if ((uint32_t)(millis() - cooldown_at) >= 2000U) { phase = READY; }
        return;
    }
    if (phase == READY) {
        if (send_text("Teste pronto. Solte USER e aguarde o LED acender.\r\n")) {
            phase = READY_TX;
        }
        return;
    }
    if (phase == ARM) {
        if (reaction_board_start()) {
            stimulus_reported = false;
            phase = RUN;
        }
        return;
    }

    reaction_status_t status = reaction_board_status();
    if (status.phase == REACTION_EARLY) {
        if (send_text("Muito cedo!\r\n")) { phase = RESULT_TX; }
    } else if ((status.phase == REACTION_STIMULUS || status.phase == REACTION_RESULT) &&
               !stimulus_reported) {
        if (send_text("LED aceso! Pressione USER.\r\n")) { phase = STIMULUS_TX; }
    } else if (status.phase == REACTION_RESULT) {
        if (send_result(status.reaction_ms)) { phase = RESULT_TX; }
    }
}
