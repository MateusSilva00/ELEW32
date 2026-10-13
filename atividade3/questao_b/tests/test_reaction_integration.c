#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "reaction_control.h"
#include "reaction_board.h"
#include "reaction_report.h"
#include "usb_serial.h"
#include "timebase.h"

static reaction_control_t board;
static uint32_t now;
static uint32_t terminal;
static usb_serial_tx_result_t tx;
static bool accept_tx;
static char line[USB_SERIAL_TX_CAPACITY + 1U];
static unsigned int sends;

/* Hardware/USB simulados. Debounce, maquina de estados e consumidor de serial
 * sao os mesmos fontes C do firmware. Nao simula NVIC nem o middleware ST. */
uint32_t millis(void) { return now; }
uint32_t usb_serial_session(void) { return terminal; }
usb_serial_tx_result_t usb_serial_take_result(void)
{
    usb_serial_tx_result_t result = tx;
    if (tx == USB_SERIAL_TX_DONE || tx == USB_SERIAL_TX_ABORTED) { tx = USB_SERIAL_TX_NONE; }
    return result;
}
bool usb_serial_write(const void *data, uint16_t length)
{
    if (!accept_tx || terminal == 0U || tx != USB_SERIAL_TX_NONE) { return false; }
    assert(length <= USB_SERIAL_TX_CAPACITY);
    memcpy(line, data, length);
    line[length] = '\0';
    ++sends;
    tx = USB_SERIAL_TX_BUSY;
    return true;
}
void reaction_board_new_session(void) { reaction_logic_init(&board.logic, 42U); }
void reaction_board_reset(void) { reaction_logic_reset(&board.logic); }
bool reaction_board_start(void) { return reaction_control_start(&board, now); }
reaction_status_t reaction_board_status(void)
{
    reaction_status_t result = {board.logic.phase, board.logic.reaction_ms};
    return result;
}

static void tick(uint32_t time)
{
    now = time;
    reaction_control_tick(&board, now);
    reaction_control_stimulus(&board, now);
}
static void edge(bool pressed, uint32_t time)
{
    now = time;
    reaction_control_edge(&board, pressed, now);
}
static void setup(void)
{
    reaction_control_init(&board, false, 0U);
    terminal = 0U;
    tx = USB_SERIAL_TX_NONE;
    accept_tx = true;
    sends = 0U;
    line[0] = '\0';
    reaction_report_poll();
    tick(30U);
    terminal = 1U;
    reaction_report_poll(); /* Nova sessao nao inicia uma rodada. */
    assert(board.logic.phase == REACTION_IDLE);
}
static void start_round(void)
{
    reaction_report_poll();
    assert(strstr(line, "Teste pronto.") != NULL && tx == USB_SERIAL_TX_BUSY);
    reaction_report_poll();
    assert(board.logic.phase == REACTION_IDLE); /* Aguarda TX_DONE. */
    tx = USB_SERIAL_TX_DONE;
    reaction_report_poll();
    assert(board.logic.phase == REACTION_IDLE);
    reaction_report_poll();
    assert(board.logic.phase == REACTION_WAITING);
}

static void test_debounce_and_boundary(void)
{
    reaction_control_t state;
    reaction_control_init(&state, true, 0U); /* Boot com USER pressionado. */
    reaction_control_tick(&state, 100U);
    assert(!reaction_control_start(&state, 100U));
    reaction_control_edge(&state, false, 200U);
    reaction_control_tick(&state, 229U);
    assert(!reaction_control_start(&state, 229U));
    reaction_control_tick(&state, 230U);
    assert(reaction_control_start(&state, 230U));
    uint32_t due = state.logic.wait_started_at + state.logic.wait_ms;
    reaction_control_edge(&state, true, due); /* Mesmo ms do prazo, antes do LED. */
    assert(!reaction_control_stimulus(&state, due));
    reaction_control_tick(&state, due + 29U);
    assert(state.logic.phase == REACTION_WAITING);
    reaction_control_tick(&state, due + 30U);
    assert(state.logic.phase == REACTION_EARLY);
    assert(!reaction_control_stimulus(&state, due + 30U));
    reaction_control_tick(&state, due + 5000U);
    assert(state.logic.phase == REACTION_EARLY);

    reaction_logic_reset(&state.logic);
    assert(!reaction_control_start(&state, due + 5000U));
    reaction_control_edge(&state, false, due + 5001U);
    reaction_control_tick(&state, due + 5031U);
    assert(reaction_control_start(&state, due + 5031U));
    due = state.logic.wait_started_at + state.logic.wait_ms;
    reaction_control_edge(&state, true, due - 10U);
    reaction_control_edge(&state, false, due + 19U); /* Pulso de 29 ms: rejeita. */
    reaction_control_tick(&state, due + 48U);
    assert(!reaction_control_stimulus(&state, due + 48U));
    reaction_control_tick(&state, due + 49U);
    assert(reaction_control_stimulus(&state, due + 49U));
    uint32_t led = state.logic.stimulus_at;
    reaction_control_edge(&state, true, led + 280U);
    reaction_control_edge(&state, false, led + 282U);
    reaction_control_edge(&state, true, led + 284U); /* Inicio estavel validado. */
    reaction_control_tick(&state, led + 313U);
    assert(state.logic.phase == REACTION_STIMULUS);
    reaction_control_tick(&state, led + 314U);
    assert(state.logic.phase == REACTION_RESULT && state.logic.reaction_ms == 284U);
    reaction_control_edge(&state, false, led + 400U);
    reaction_control_tick(&state, led + 430U);
    assert(state.logic.reaction_ms == 284U && state.logic.phase == REACTION_RESULT);

    reaction_control_init(&state, false, UINT32_MAX - 100U);
    reaction_control_tick(&state, UINT32_MAX - 70U);
    assert(reaction_control_start(&state, UINT32_MAX - 70U));
    reaction_control_edge(&state, true, UINT32_MAX - 10U);
    reaction_control_tick(&state, 18U);
    assert(state.logic.phase == REACTION_WAITING);
    reaction_control_tick(&state, 19U);
    assert(state.logic.phase == REACTION_EARLY);
}

static void test_serial_result_and_hold(void)
{
    setup();
    accept_tx = false;
    reaction_report_poll();
    assert(sends == 0U && board.logic.phase == REACTION_IDLE);
    accept_tx = true;
    start_round();
    uint32_t led = board.logic.wait_started_at + board.logic.wait_ms;
    tick(led);
    reaction_report_poll();
    assert(strcmp(line, "LED aceso! Pressione USER.\r\n") == 0);
    edge(true, led + 284U);
    tick(led + 314U);
    reaction_report_poll(); /* Mensagem de estimulo ainda ocupada. */
    assert(sends == 2U && board.logic.reaction_ms == 284U);
    tx = USB_SERIAL_TX_DONE;
    reaction_report_poll();
    reaction_report_poll();
    assert(strcmp(line, "Tempo de reacao: 284 ms\r\n") == 0 && sends == 3U);
    tick(led + 1000U);
    reaction_report_poll();
    assert(sends == 3U && board.logic.phase == REACTION_RESULT);
    tx = USB_SERIAL_TX_ABORTED;
    reaction_report_poll();
    assert(board.logic.phase == REACTION_RESULT);
    reaction_report_poll();
    assert(sends == 4U && strcmp(line, "Tempo de reacao: 284 ms\r\n") == 0);
    tx = USB_SERIAL_TX_DONE;
    reaction_report_poll();
    assert(board.logic.phase == REACTION_IDLE);
    tick(led + 2999U);
    reaction_report_poll();
    assert(sends == 4U);
    tick(led + 3000U);
    reaction_report_poll();
    reaction_report_poll();
    assert(sends == 5U && strstr(line, "Teste pronto.") != NULL);
    tx = USB_SERIAL_TX_DONE;
    reaction_report_poll();
    reaction_report_poll();
    assert(board.logic.phase == REACTION_IDLE); /* Ainda mantem USER apertado. */
    edge(false, led + 5000U);
    tick(led + 5029U);
    reaction_report_poll();
    assert(board.logic.phase == REACTION_IDLE);
    tick(led + 5030U);
    reaction_report_poll();
    assert(board.logic.phase == REACTION_WAITING);
}

static void test_early_and_sessions(void)
{
    setup();
    start_round();
    edge(true, now + 1U);
    tick(now + 30U);
    reaction_report_poll();
    assert(strcmp(line, "Muito cedo!\r\n") == 0 && sends == 2U);
    /* Fecha com resultado em voo: cancela mesmo enquanto USB esta BUSY. */
    terminal = 0U;
    reaction_report_poll();
    assert(board.logic.phase == REACTION_IDLE);
    terminal = 2U;
    reaction_report_poll();
    reaction_report_poll();
    assert(sends == 2U);
    tx = USB_SERIAL_TX_DONE; /* Conclusao antiga nao inicia a nova rodada. */
    reaction_report_poll();
    assert(sends == 3U && strstr(line, "Teste pronto.") != NULL);
    assert(board.logic.phase == REACTION_IDLE);
    tx = USB_SERIAL_TX_ABORTED;
    reaction_report_poll();
    reaction_report_poll();
    assert(sends == 4U && strstr(line, "Teste pronto.") != NULL);
    tx = USB_SERIAL_TX_DONE;
    reaction_report_poll();
    edge(false, now + 10U);
    tick(now + 30U);
    reaction_report_poll();
    assert(board.logic.phase == REACTION_WAITING);
    /* Fecha/reabre entre dois polls: ID diferente deve cancelar a espera. */
    terminal = 3U;
    reaction_report_poll();
    assert(board.logic.phase == REACTION_IDLE);
    start_round();
    tick(board.logic.wait_started_at + board.logic.wait_ms);
    reaction_report_poll();
    assert(board.logic.phase == REACTION_STIMULUS);
    terminal = 0U;
    reaction_report_poll();
    assert(board.logic.phase == REACTION_IDLE);
}

void test_reaction_integration(void)
{
    test_debounce_and_boundary();
    puts("PASS: debounce 29/30 ms, bouncing, boot pressionado e antecipacao no prazo");
    test_serial_result_and_hold();
    puts("PASS: serial em ordem, USB ocupado/abortado, timestamp e botao mantido");
    test_early_and_sessions();
    puts("PASS: Muito cedo, cancelamento/reabertura e conclusao USB de sessao antiga");
}
