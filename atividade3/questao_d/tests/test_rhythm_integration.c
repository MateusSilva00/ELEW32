#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "rhythm_control.h"
#include "rhythm_board.h"
#include "rhythm_report.h"
#include "timebase.h"
#include "usb_serial.h"

static rhythm_control_t board;
static uint32_t now;
static uint32_t terminal;
static usb_serial_tx_result_t tx;
static bool accept_tx;
static char line[USB_SERIAL_TX_CAPACITY + 1U];
static unsigned int sends;
static uint16_t longest;

/* Adaptadores simulados; logica, debounce, fila e relatorio sao reais. */
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
    if (length > longest) { longest = length; }
    memcpy(line, data, length);
    line[length] = '\0';
    ++sends;
    tx = USB_SERIAL_TX_BUSY;
    return true;
}
void rhythm_board_session(bool active)
{
    rhythm_control_init(&board, active, board.raw_pressed, now, 1U);
}
rhythm_sequence_t rhythm_board_sequence(void)
{
    rhythm_sequence_t sequence;
    sequence.steps = board.logic.steps;
    memcpy(sequence.pattern, board.logic.pattern, sizeof(sequence.pattern));
    return sequence;
}
bool rhythm_board_start(void) { return rhythm_control_start(&board, now); }
bool rhythm_board_next(void) { return rhythm_logic_next(&board.logic); }
bool rhythm_board_peek(rhythm_event_t *event) { return rhythm_control_peek(&board, event); }
void rhythm_board_pop(void) { rhythm_control_pop(&board); }

static const rhythm_class_t S = RHYTHM_SHORT;
static const rhythm_class_t L = RHYTHM_LONG;

static void tick(uint32_t at) { now = at; rhythm_control_tick(&board, at); }
static void edge(bool pressed, uint32_t at) { now = at; rhythm_control_edge(&board, pressed, at); }
static void run_until(uint32_t at)
{
    while (now != at) { tick(now + 1U); }
}
/* Clique completo: pressao em at, soltura 80 ms depois, ambos validados. */
static void click(uint32_t at)
{
    run_until(at);
    edge(true, at);
    run_until(at + 30U);
    edge(false, at + 80U);
    run_until(at + 110U);
}
static void pattern(const rhythm_class_t *values, unsigned int steps)
{
    board.logic.steps = (uint8_t)steps;
    memcpy(board.logic.pattern, values, steps * sizeof(values[0]));
}
static uint32_t demo_end(void)
{
    uint32_t end = board.logic.demo_at + RHYTHM_FLASH_MS;
    for (unsigned int index = 0U; index < board.logic.steps; ++index) {
        end += rhythm_logic_nominal_ms(board.logic.pattern[index]);
    }
    return end;
}
static void ready_round(void)
{
    now = 0U;
    rhythm_control_init(&board, true, false, now, 1U);
    const rhythm_class_t values[] = {S, L, S};
    pattern(values, 3U);
    run_until(30U);
    assert(board.ready && rhythm_control_start(&board, now));
}
static rhythm_event_t peek(void)
{
    rhythm_event_t event;
    assert(rhythm_control_peek(&board, &event));
    return event;
}

static void test_debounce_and_led(void)
{
    /* Boot com USER pressionado: so fica pronto apos soltura estavel. */
    rhythm_control_init(&board, true, true, 0U, 1U);
    run_until(5000U);
    assert(!board.ready);
    edge(false, 5100U);
    run_until(5129U);
    assert(!board.ready);
    tick(5130U);
    assert(board.ready);

    ready_round(); /* Demo em 1030: batidas 1030, 1330, 2230, 2530. */
    assert(!rhythm_control_led(&board, 1029U) && rhythm_control_led(&board, 1030U));
    assert(rhythm_control_led(&board, 2530U) && !rhythm_control_led(&board, 2630U));
    click(1500U); /* Durante a demonstracao: ignorado, sem LED de retorno. */
    assert(board.count == 0U && board.logic.phase == RHYTHM_DEMO);
    run_until(demo_end());
    assert(board.logic.phase == RHYTHM_WAIT);

    uint32_t start = now + 200U;
    run_until(start);
    edge(true, start);
    tick(start + 29U);
    assert(!rhythm_control_led(&board, now));
    tick(start + 30U);
    assert(rhythm_control_led(&board, now)); /* LED acompanha a pressao validada. */
    assert(board.logic.phase == RHYTHM_INPUT && board.logic.last_click_at == start);
    edge(false, start + 80U);
    run_until(start + 110U);
    assert(!rhythm_control_led(&board, now));

    /* Pulso de 29 ms rejeitado; bouncing preserva o instante da ultima borda. */
    edge(true, start + 200U);
    tick(start + 229U);
    edge(false, start + 229U);
    run_until(start + 259U);
    assert(board.count == 0U);
    edge(true, start + 300U);
    edge(false, start + 303U);
    edge(true, start + 306U);
    run_until(start + 336U);
    assert(peek().interval_ms == 306U && peek().measured == S);
    edge(false, start + 400U);
    edge(true, start + 404U);
    edge(false, start + 408U);
    run_until(start + 438U);
    assert(board.count == 1U); /* Soltura com bouncing nao e clique. */
}

static void test_pending_edge_at_deadline(void)
{
    for (uint32_t gap = 1999U; gap <= 2000U; ++gap) {
        ready_round();
        uint32_t first = demo_end();
        click(first);
        uint32_t second = first + 300U;
        click(second);
        assert(peek().interval_ms == 300U);
        rhythm_control_pop(&board);
        run_until(second + gap - 1U);
        edge(true, second + gap); /* Pressao no limite, confirmada depois do prazo. */
        run_until(second + gap + 30U);
        rhythm_event_t event = peek();
        if (gap == 1999U) {
            assert(event.interval_ms == 1999U && event.outcome == RHYTHM_PENDING);
        } else {
            assert(event.outcome == RHYTHM_TIMEOUT && event.received == 1U);
        }
    }
    /* Sem nova pressao o prazo e detectado pelo tick. */
    ready_round();
    uint32_t first = demo_end();
    click(first);
    run_until(first + 1999U);
    assert(board.count == 0U);
    tick(first + 2000U);
    assert(peek().outcome == RHYTHM_TIMEOUT && peek().received == 0U);
    run_until(first + 9000U);
    assert(board.count == 1U);
}

static void poll_until_tx(void)
{
    unsigned int before = sends;
    for (unsigned int guard = 0U; sends == before; ++guard) {
        assert(guard < 100000U);
        rhythm_report_poll();
        tick(now + 1U);
    }
}
static void done(void) { tx = USB_SERIAL_TX_DONE; }

static void open_session(uint32_t id)
{
    tx = USB_SERIAL_TX_NONE;
    accept_tx = true;
    terminal = id;
    rhythm_report_poll();
    assert(board.active);
    const rhythm_class_t values[] = {S, L, S};
    pattern(values, 3U);
}

static void test_report_round(void)
{
    terminal = 0U;
    rhythm_report_poll();
    now = 100U;
    open_session(1U);
    sends = 0U;
    poll_until_tx();
    assert(strcmp(line, "Nivel 1 (3 trechos): curto longo curto\r\n") == 0);
    accept_tx = false;
    done();
    rhythm_report_poll();
    rhythm_report_poll();
    assert(sends == 1U); /* USB ocupado mantem a dica pendente. */
    accept_tx = true;
    rhythm_report_poll();
    assert(strcmp(line, "Observe o LED; depois repita com 4 cliques.\r\n") == 0);
    tx = USB_SERIAL_TX_ABORTED;
    rhythm_report_poll();
    rhythm_report_poll();
    assert(sends == 3U && strcmp(line, "Observe o LED; depois repita com 4 cliques.\r\n") == 0);
    assert(board.logic.phase == RHYTHM_IDLE); /* Demo so apos a dica transmitida. */
    done();
    rhythm_report_poll();
    assert(board.logic.phase == RHYTHM_DEMO);

    uint32_t at = demo_end();
    run_until(at);
    click(at);
    click(at + 320U);
    poll_until_tx();
    assert(strcmp(line, "Intervalo 1/3: 320 ms -> curto (ok)\r\n") == 0);
    done();
    click(at + 320U + 880U);
    poll_until_tx();
    assert(strcmp(line, "Intervalo 2/3: 880 ms -> longo (ok)\r\n") == 0);
    done();
    click(at + 1200U + 650U);
    poll_until_tx();
    assert(strcmp(line, "Intervalo 3/3: 650 ms -> longo (esperado curto)\r\n"
                        "Tentativa incorreta. Repita a mesma sequencia.\r\n") == 0);
    done();
    rhythm_report_poll();
    uint32_t paused = now;
    while (now - paused < RHYTHM_PAUSE_MS - 1U) {
        rhythm_report_poll();
        tick(now + 1U);
        assert(board.logic.phase == RHYTHM_DONE);
    }
    poll_until_tx();
    assert(now - paused >= RHYTHM_PAUSE_MS);
    assert(strcmp(line, "Nivel 1 (3 trechos): curto longo curto\r\n") == 0);

    /* Correta: proximo nivel com o mesmo prefixo. */
    done();
    poll_until_tx();
    done();
    rhythm_report_poll();
    at = demo_end();
    run_until(at);
    click(at);
    click(at + 300U);
    click(at + 1200U);
    click(at + 1500U);
    for (unsigned int index = 0U; index < 3U; ++index) {
        poll_until_tx();
        done();
    }
    assert(strcmp(line, "Intervalo 3/3: 300 ms -> curto (ok)\r\n"
                        "Tentativa correta! Proximo nivel com mais um trecho.\r\n") == 0);
    poll_until_tx();
    assert(strncmp(line, "Nivel 2 (4 trechos): curto longo curto ", 39U) == 0);
    assert(board.logic.steps == 4U);

    /* Timeout com mensagem propria. */
    done();
    poll_until_tx();
    assert(strcmp(line, "Observe o LED; depois repita com 5 cliques.\r\n") == 0);
    done();
    rhythm_report_poll();
    at = demo_end();
    run_until(at);
    click(at);
    run_until(at + 2000U);
    poll_until_tx();
    assert(strcmp(line, "Sem clique por 2 s: 0/4 intervalos.\r\n"
                        "Tentativa incorreta. Repita a mesma sequencia.\r\n") == 0);
}

static void test_sessions_and_longest_lines(void)
{
    /* Fechar e reabrir durante uma transmissao reinicia a rodada. */
    now = 50000U;
    open_session(5U);
    poll_until_tx();
    terminal = 6U;
    rhythm_report_poll();
    assert(board.logic.phase == RHYTHM_IDLE && board.count == 0U);
    done(); /* Conclusao antiga nao avanca a nova sessao. */
    rhythm_report_poll();
    assert(board.logic.phase == RHYTHM_IDLE);
    terminal = 0U;
    rhythm_report_poll();
    assert(!board.active && !rhythm_control_led(&board, now));
    click(now + 100U);
    assert(board.count == 0U);

    /* Nivel maximo: maior anuncio e maior linha final cabem no buffer USB. */
    open_session(7U);
    rhythm_class_t values[RHYTHM_MAX_STEPS];
    for (unsigned int index = 0U; index < RHYTHM_MAX_STEPS; ++index) { values[index] = L; }
    pattern(values, RHYTHM_MAX_STEPS);
    longest = 0U;
    poll_until_tx();
    assert(strncmp(line, "Nivel 10 (12 trechos): longo longo", 34U) == 0);
    done();
    poll_until_tx();
    assert(strcmp(line, "Observe o LED; depois repita com 13 cliques.\r\n") == 0);
    done();
    rhythm_report_poll();
    uint32_t at = demo_end();
    run_until(at);
    click(at);
    for (unsigned int index = 0U; index < RHYTHM_MAX_STEPS; ++index) {
        at += 1999U;
        click(at);
        poll_until_tx();
        done();
    }
    assert(strcmp(line, "Intervalo 12/12: 1999 ms -> longo (ok)\r\n"
                        "Tentativa correta! Nivel maximo; nova sequencia de 3.\r\n") == 0);
    poll_until_tx();
    assert(strncmp(line, "Nivel 1 (3 trechos): ", 21U) == 0);
    printf("      maior linha transmitida: %u de %u bytes\n",
           (unsigned int)longest, (unsigned int)USB_SERIAL_TX_CAPACITY);
}

void test_rhythm_integration(void)
{
    test_debounce_and_led();
    puts("PASS: debounce 29/30 ms, boot pressionado, bouncing e LED de demonstracao/clique");
    test_pending_edge_at_deadline();
    puts("PASS: pressao pendente em 1999/2000 ms e prazo detectado pelo SysTick");
    test_report_round();
    puts("PASS: anuncio, intervalos, resultado, pausa de 1,5 s, USB ocupado/abortado");
    test_sessions_and_longest_lines();
    puts("PASS: troca de sessao, nivel maximo e linhas dentro do buffer USB");
}
