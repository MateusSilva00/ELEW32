#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "morse_control.h"
#include "morse_board.h"
#include "morse_report.h"
#include "usb_serial.h"

static morse_control_t board;
static uint32_t now;
static uint32_t terminal;
static usb_serial_tx_result_t tx;
static bool accept_tx;
static char line[USB_SERIAL_TX_CAPACITY + 1U];
static unsigned int sends;

/* Adaptadores simulados; logica, debounce, fila e consumidor sao reais. */
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
void morse_board_session(bool active)
{
    morse_control_init(&board, active, board.raw_pressed, now);
}
bool morse_board_peek(morse_result_t *result) { return morse_control_peek(&board, result); }
void morse_board_pop(void) { morse_control_pop(&board); }
uint32_t morse_board_lost(void) { return board.lost_total; }

static void tick(uint32_t at) { now = at; morse_control_tick(&board, at); }
static void edge(bool pressed, uint32_t at)
{
    now = at;
    morse_control_edge(&board, pressed, at);
}
static void ready(void)
{
    now = 0U;
    morse_control_init(&board, true, false, now);
    tick(30U);
    assert(board.ready);
}
static uint32_t symbol(uint32_t at, uint32_t duration)
{
    edge(true, at);
    tick(at + 30U);
    edge(false, at + duration);
    tick(at + duration + 30U);
    return at + duration;
}
static void letter(const char *symbols)
{
    uint32_t release = now;
    while (*symbols) { release = symbol(now + 100U, *symbols++ == '.' ? 100U : 600U); }
    tick(release + 1000U);
}
static morse_result_t peek(void)
{
    morse_result_t result;
    assert(morse_control_peek(&board, &result));
    return result;
}

static void test_boot_bounce_and_thresholds(void)
{
    morse_control_init(&board, true, true, 0U);
    tick(5000U);
    assert(!board.ready && board.count == 0U);
    edge(false, 5100U);
    tick(5129U);
    assert(!board.ready);
    tick(5130U);
    assert(board.ready && board.count == 0U);
    for (uint32_t duration = 299U; duration <= 300U; ++duration) {
        ready();
        uint32_t release = symbol(100U, duration);
        tick(release + 999U);
        assert(board.count == 0U);
        tick(release + 1000U);
        morse_result_t result = peek();
        assert(result.letter == (duration == 299U ? 'E' : 'T'));
        assert(result.finished_at == release + 1000U);
    }
    ready();
    edge(true, 100U);
    tick(129U);
    edge(false, 129U); /* Pulso de 29 ms rejeitado. */
    tick(159U);
    tick(2000U);
    assert(board.count == 0U);
    edge(true, 2100U);
    edge(false, 2105U);
    edge(true, 2110U);
    tick(2140U);
    edge(false, 2300U);
    edge(true, 2305U);
    edge(false, 2310U);
    tick(2340U);
    tick(3310U);
    assert(peek().letter == 'E'); /* Duracao entre as ultimas bordas estaveis. */
    morse_control_pop(&board);
    edge(true, 4000U);
    tick(4030U);
    tick(14000U); /* Mantido nao encerra. */
    assert(board.count == 0U);
    edge(false, 14000U);
    tick(14030U);
    tick(15000U);
    assert(peek().letter == 'T' && board.count == 1U);
    tick(20000U);
    assert(board.count == 1U);
}

static void test_pending_edge_and_wrap(void)
{
    for (uint32_t gap = 999U; gap <= 1000U; ++gap) {
        ready();
        symbol(100U, 100U); /* Soltura em 200, prazo em 1200. */
        edge(true, 200U + gap);
        tick(1229U);
        assert(board.count == 0U);
        tick(230U + gap);
        assert(board.count == (gap == 999U ? 0U : 1U));
        if (gap == 1000U) { assert(peek().letter == 'E'); morse_control_pop(&board); }
        edge(false, 800U + gap);
        tick(830U + gap);
        tick(1800U + gap);
        assert(peek().letter == (gap == 999U ? 'A' : 'T'));
    }
    ready();
    symbol(100U, 100U);
    edge(true, 1199U); /* Ruido perto do prazo. */
    tick(1200U);
    assert(board.count == 0U);
    edge(false, 1205U);
    tick(1235U);
    assert(peek().letter == 'E' && peek().finished_at == 1200U);

    morse_control_init(&board, true, false, UINT32_MAX - 500U);
    tick(UINT32_MAX - 470U);
    uint32_t release = symbol(UINT32_MAX - 100U, 300U);
    tick(release + 1000U);
    assert(peek().letter == 'T' && peek().finished_at == 1199U);
}

static void setup_report(void)
{
    ready();
    terminal = 0U;
    tx = USB_SERIAL_TX_NONE;
    accept_tx = true;
    sends = 0U;
    morse_report_poll();
    terminal = 1U;
    morse_report_poll();
    tick(now + 30U);
}

static void test_usb_formats_and_sessions(void)
{
    setup_report();
    letter(".-");
    accept_tx = false;
    morse_report_poll();
    assert(sends == 0U && board.count == 1U);
    accept_tx = true;
    morse_report_poll();
    assert(strcmp(line, "Morse .- -> A\r\n") == 0);
    morse_report_poll();
    assert(board.count == 1U && sends == 1U);
    tx = USB_SERIAL_TX_ABORTED;
    morse_report_poll();
    assert(board.count == 1U && sends == 2U);
    tx = USB_SERIAL_TX_DONE;
    morse_report_poll();
    assert(board.count == 0U);
    letter("----");
    morse_report_poll();
    assert(strcmp(line, "Morse ---- -> sequencia invalida\r\n") == 0);
    tx = USB_SERIAL_TX_DONE;
    morse_report_poll();
    letter(".....");
    morse_report_poll();
    assert(strcmp(line, "Morse (prefixo): .... -> mais de 4 simbolos\r\n") == 0);

    /* Abre outra sessao entre polls com mensagem antiga em voo. */
    terminal = 2U;
    morse_report_poll();
    assert(board.count == 0U && !board.ready);
    tick(now + 30U);
    letter(".");
    tx = USB_SERIAL_TX_DONE;
    morse_report_poll();
    assert(board.count == 1U && strcmp(line, "Morse . -> E\r\n") == 0);
    terminal = 0U;
    morse_report_poll();
    assert(!board.active && board.count == 0U);
    letter("-");
    assert(board.count == 0U);
    /* Cancela uma letra parcial ao fechar. */
    terminal = 3U;
    tx = USB_SERIAL_TX_ABORTED;
    morse_report_poll();
    tick(now + 30U);
    symbol(now + 100U, 100U);
    assert(board.logic.count == 1U);
    terminal = 0U;
    morse_report_poll();
    assert(board.logic.count == 0U);
}

static void test_queue_and_losses(void)
{
    setup_report();
    accept_tx = false;
    for (unsigned int i = 0U; i < 18U; ++i) { letter(i % 2U == 0U ? "." : "-"); }
    assert(board.count == 16U && board.lost_total == 2U);
    accept_tx = true;
    morse_report_poll();
    for (unsigned int i = 0U; i < 16U; ++i) {
        assert(strcmp(line, i % 2U == 0U ? "Morse . -> E\r\n" : "Morse - -> T\r\n") == 0);
        tx = USB_SERIAL_TX_DONE;
        morse_report_poll();
    }
    assert(strcmp(line, "Resultados perdidos: 2 (fila cheia)\r\n") == 0);
    /* Mais perdas enquanto o primeiro aviso ainda esta em voo. */
    for (unsigned int i = 0U; i < 17U; ++i) { letter("."); }
    tx = USB_SERIAL_TX_DONE;
    morse_report_poll();
    for (unsigned int i = 0U; i < 16U; ++i) {
        assert(strcmp(line, "Morse . -> E\r\n") == 0);
        tx = USB_SERIAL_TX_DONE;
        morse_report_poll();
    }
    assert(strcmp(line, "Resultados perdidos: 1 (fila cheia)\r\n") == 0);
    tx = USB_SERIAL_TX_DONE;
    morse_report_poll();
    unsigned int count = sends;
    morse_report_poll();
    assert(sends == count && board.count == 0U);
    for (unsigned int i = 0U; i < 1000U; ++i) {
        letter(".");
        assert(peek().letter == 'E');
        morse_control_pop(&board);
    }
    assert(board.count == 0U && board.lost_total == 3U);
}

void test_morse_integration(void)
{
    test_boot_bounce_and_thresholds();
    puts("PASS: debounce 29/30 ms, boot pressionado, bouncing e duracao preservada");
    test_pending_edge_and_wrap();
    puts("PASS: borda pendente em 999/1000 ms, pulso rejeitado e wrap com debounce");
    test_usb_formats_and_sessions();
    puts("PASS: formato de letras/erros, USB ocupado/abortado e troca de sessao");
    test_queue_and_losses();
    puts("PASS: FIFO de 16, perdas durante aviso e 1000 ciclos de reutilizacao");
}
