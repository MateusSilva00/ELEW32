#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "button_logic.h"
#include "button.h"
#include "button_report.h"
#include "usb_serial.h"

static button_logic_t model;
static usb_serial_tx_result_t tx_result;
static bool usb_ready;
static char transmitted[USB_SERIAL_TX_CAPACITY + 1U];
static unsigned int sends;

/* Adaptador simulado apenas para testar o consumidor real da fila. */
bool button_event_peek(button_event_t *event) { return button_logic_peek(&model, event); }
void button_event_pop(void) { button_logic_pop(&model); }
uint32_t button_lost_total(void) { return model.lost_total; }
bool usb_serial_write(const void *data, uint16_t length)
{
    if (!usb_ready || tx_result != USB_SERIAL_TX_NONE) { return false; }
    assert(length <= USB_SERIAL_TX_CAPACITY);
    memcpy(transmitted, data, length);
    transmitted[length] = '\0';
    ++sends;
    tx_result = USB_SERIAL_TX_BUSY;
    return true;
}
usb_serial_tx_result_t usb_serial_take_result(void)
{
    usb_serial_tx_result_t result = tx_result;
    if (result == USB_SERIAL_TX_DONE || result == USB_SERIAL_TX_ABORTED) {
        tx_result = USB_SERIAL_TX_NONE;
    }
    return result;
}

static void ready(button_logic_t *state)
{
    button_logic_init(state, false, 0U);
    button_logic_tick(state, 29U);
    assert(!state->armed);
    button_logic_tick(state, 30U);
    assert(state->armed && state->count == 0U);
}

static void click(button_logic_t *state, uint32_t start)
{
    button_logic_edge(state, true, start);
    button_logic_tick(state, start + 30U);
    button_logic_edge(state, false, start + 40U);
    button_logic_tick(state, start + 70U);
}

static void test_debounce(void)
{
    button_logic_t state;
    button_event_t event;
    ready(&state);
    button_logic_edge(&state, true, 100U);
    button_logic_tick(&state, 129U);
    assert(state.count == 0U);
    button_logic_edge(&state, false, 129U); /* Pulso de 29 ms rejeitado. */
    button_logic_tick(&state, 159U);
    assert(state.count == 0U);

    button_logic_edge(&state, true, 200U);
    button_logic_edge(&state, false, 204U);
    button_logic_edge(&state, true, 211U);
    button_logic_tick(&state, 240U);
    assert(state.count == 0U);
    button_logic_tick(&state, 241U);
    assert(button_logic_peek(&state, &event));
    assert(event.number == 1U && event.timestamp_ms == 211U);
    button_logic_tick(&state, 10000U); /* Mantido: nao repete. */
    assert(state.count == 1U);

    button_logic_edge(&state, false, 10001U);
    button_logic_tick(&state, 10030U); /* Soltura de 29 ms nao rearma. */
    button_logic_edge(&state, true, 10030U);
    button_logic_tick(&state, 10060U);
    assert(state.count == 1U);
    button_logic_edge(&state, false, 10100U);
    button_logic_tick(&state, 10130U); /* Soltura estavel permite novo clique. */
    assert(state.count == 1U);
    click(&state, 10200U);
    assert(state.count == 2U && state.press_count == 2U);
    button_logic_pop(&state);
    assert(button_logic_peek(&state, &event));
    assert(event.number == 2U && event.timestamp_ms == 10200U);

    /* Atendimentos com mesmo nivel tambem reiniciam o periodo de estabilidade. */
    ready(&state);
    button_logic_edge(&state, true, 100U);
    button_logic_edge(&state, true, 120U);
    button_logic_tick(&state, 149U);
    assert(state.count == 0U);
    button_logic_tick(&state, 150U);
    assert(state.count == 1U && state.events[0].timestamp_ms == 120U);
}

static void test_boot_and_wrap(void)
{
    button_logic_t state;
    button_logic_init(&state, true, 0U);
    button_logic_tick(&state, 100U);
    assert(state.count == 0U && !state.armed);
    button_logic_edge(&state, false, 101U);
    button_logic_tick(&state, 131U);
    click(&state, 200U);
    assert(state.count == 1U);

    ready(&state);
    uint32_t edge = UINT32_MAX - 10U;
    button_logic_edge(&state, true, edge);
    button_logic_tick(&state, 18U);
    assert(state.count == 0U); /* 29 ms atraves da volta a zero. */
    button_logic_tick(&state, 19U);
    assert(state.count == 1U && state.events[0].timestamp_ms == edge);
    button_logic_tick(&state, 200U);
    assert(state.count == 1U);
}

static void test_queue(void)
{
    button_logic_t state;
    button_event_t event;
    ready(&state);
    assert(!button_logic_peek(&state, &event));
    button_logic_pop(&state);
    for (uint32_t i = 0U; i < 35U; ++i) { click(&state, 100U + i * 100U); }
    assert(state.count == 32U && state.press_count == 35U && state.lost_total == 3U);
    for (uint32_t i = 1U; i <= 32U; ++i) {
        assert(button_logic_peek(&state, &event));
        assert(event.number == i && event.timestamp_ms == i * 100U);
        button_logic_pop(&state);
    }
    assert(!button_logic_peek(&state, &event));
    for (uint32_t i = 0U; i < 1000U; ++i) {
        click(&state, 4000U + i * 100U);
        assert(button_logic_peek(&state, &event));
        assert(event.number == 36U + i && event.timestamp_ms == 4000U + i * 100U);
        button_logic_pop(&state);
    }
    assert(state.count == 0U && state.lost_total == 3U);
}

static void test_reporting(void)
{
    ready(&model);
    click(&model, 2387U);
    usb_ready = false;
    button_report_poll();
    assert(sends == 0U && model.count == 1U);
    usb_ready = true;
    button_report_poll();
    assert(strcmp(transmitted, "Botao 1 pressionado em 2387 ms\r\n") == 0);
    assert(sends == 1U && model.count == 1U);
    button_report_poll();
    assert(sends == 1U && model.count == 1U); /* Busy nao remove nem sobrescreve. */
    tx_result = USB_SERIAL_TX_ABORTED;
    usb_ready = false;
    button_report_poll();
    assert(model.count == 1U);
    usb_ready = true;
    button_report_poll();
    assert(sends == 2U && strcmp(transmitted, "Botao 1 pressionado em 2387 ms\r\n") == 0);
    tx_result = USB_SERIAL_TX_DONE;
    button_report_poll();
    assert(model.count == 0U && sends == 2U);

    for (uint32_t i = 0U; i < 34U; ++i) { click(&model, 3000U + i * 100U); }
    assert(model.lost_total == 2U);
    button_report_poll();
    for (uint32_t i = 0U; i < 32U; ++i) {
        assert(strncmp(transmitted, "Botao ", 6U) == 0);
        tx_result = USB_SERIAL_TX_DONE;
        button_report_poll();
    }
    assert(strcmp(transmitted, "Eventos perdidos: 2 (fila cheia)\r\n") == 0);
    /* Mais perdas enquanto o aviso anterior esta em transito. */
    for (uint32_t i = 0U; i < 33U; ++i) { click(&model, 7000U + i * 100U); }
    assert(model.lost_total == 3U);
    tx_result = USB_SERIAL_TX_DONE;
    button_report_poll();
    for (uint32_t i = 0U; i < 32U; ++i) {
        tx_result = USB_SERIAL_TX_DONE;
        button_report_poll();
    }
    assert(strcmp(transmitted, "Eventos perdidos: 1 (fila cheia)\r\n") == 0);
    tx_result = USB_SERIAL_TX_ABORTED;
    button_report_poll();
    assert(strcmp(transmitted, "Eventos perdidos: 1 (fila cheia)\r\n") == 0);
    tx_result = USB_SERIAL_TX_DONE;
    button_report_poll();
    unsigned int previous = sends;
    button_report_poll();
    assert(sends == previous && model.count == 0U);
}

int main(void)
{
    test_debounce();
    puts("PASS: 29/30 ms, bouncing, timestamp, botao mantido e soltura estavel");
    test_boot_and_wrap();
    puts("PASS: boot pressionado e wrap de uint32_t");
    test_queue();
    puts("PASS: 32 vagas, overflow, ordem FIFO e reutilizacao circular");
    test_reporting();
    puts("PASS: USB ocupado/abortado/concluido, mensagens e avisos de perda");
    return 0;
}
