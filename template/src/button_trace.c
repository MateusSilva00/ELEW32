#include "button_trace.h"
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

void button_trace_poll(void)
{
    static bool reported;
    static uint32_t last_reported;
    button_snapshot_t snapshot = button_snapshot();
    if (reported && snapshot.irq_count == last_reported) {
        return;
    }

    /* Pior caso: 49 caracteres fixos + 3 numeros de ate 10 digitos = 79.
     * Transmitimos o tamanho explicito, sem precisar de terminador NUL. */
    char line[96];
    _Static_assert(sizeof(line) <= USB_SERIAL_TX_CAPACITY, "Linha excede buffer USB");
    char *end = append_text(line, snapshot.pressed ?
                           "Botao: PA0=0 (pressionado), irq=" :
                           "Botao: PA0=1 (solto), irq=");
    end = append_u32(end, snapshot.irq_count);
    end = append_text(end, ", t=");
    end = append_u32(end, snapshot.timestamp_ms);
    end = append_text(end, " ms, delta=");
    end = append_u32(end, snapshot.irq_count - last_reported);
    end = append_text(end, "\r\n");

    /* USB ocupado: preserva o numero do ultimo envio aceito e tenta depois.
     * O proximo registro pode ser mais recente: ainda nao existe fila. */
    if (usb_serial_write(line, (uint16_t)(end - line))) {
        last_reported = snapshot.irq_count;
        reported = true;
    }
}
