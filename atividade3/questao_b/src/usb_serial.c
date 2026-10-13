#include "usb_serial.h"
#include "usbd_core.h"
#include "usbd_cdc.h"

extern USBD_DescriptorsTypeDef usb_descriptors;
static USBD_HandleTypeDef device;
static USBD_CDC_HandleTypeDef class_storage;
static bool class_allocated;
static uint8_t rx_buffer[CDC_DATA_FS_MAX_PACKET_SIZE];
static uint8_t tx_buffer[USB_SERIAL_TX_CAPACITY];
static uint8_t line_coding[7] = {0x00, 0xC2, 0x01, 0x00, 0, 0, 8}; /* 115200 8N1 */
static volatile bool terminal_open;
static volatile bool greeting_pending;
static volatile usb_serial_tx_result_t app_result;

/* Somente uma instancia CDC; sem heap e sem malloc dentro da interrupcao. */
void *USBD_static_malloc(uint32_t size)
{
    if (class_allocated || size > sizeof(class_storage)) {
        return NULL;
    }
    class_allocated = true;
    return &class_storage;
}

void USBD_static_free(void *ptr)
{
    if (ptr == &class_storage) {
        class_allocated = false;
    }
}

static int8_t cdc_init(void)
{
    terminal_open = false;
    greeting_pending = true;
    USBD_CDC_SetTxBuffer(&device, tx_buffer, 0U);
    USBD_CDC_SetRxBuffer(&device, rx_buffer);
    return USBD_OK;
}

static int8_t cdc_deinit(void)
{
    if (app_result == USB_SERIAL_TX_BUSY) {
        app_result = USB_SERIAL_TX_ABORTED;
    }
    terminal_open = false;
    greeting_pending = true;
    return USBD_OK;
}

static int8_t cdc_control(uint8_t command, uint8_t *buffer, uint16_t length)
{
    switch (command) {
    case CDC_SET_LINE_CODING:
        if (length != sizeof(line_coding)) {
            return USBD_FAIL;
        }
        memcpy(line_coding, buffer, sizeof(line_coding));
        break;
    case CDC_GET_LINE_CODING:
        if (length < sizeof(line_coding)) {
            return USBD_FAIL;
        }
        memcpy(buffer, line_coding, sizeof(line_coding));
        break;
    case CDC_SET_CONTROL_LINE_STATE: {
        /* Nesta versao ST, pedidos sem dados entregam o setup inteiro. */
        const USBD_SetupReqTypedef *request = (const USBD_SetupReqTypedef *)buffer;
        bool dtr = (request->wValue & 1U) != 0U;
        if (dtr && !terminal_open) {
            greeting_pending = true;
        }
        terminal_open = dtr;
        break;
    }
    default:
        break;
    }
    return USBD_OK;
}

static int8_t cdc_receive(uint8_t *buffer, uint32_t *length)
{
    (void)buffer;
    (void)length;
    /* A atividade apenas transmite. Descarta entrada e rearma o endpoint. */
    USBD_CDC_ReceivePacket(&device);
    return USBD_OK;
}

static int8_t cdc_transmit_complete(uint8_t *buffer, uint32_t *length, uint8_t endpoint)
{
    (void)buffer;
    (void)length;
    (void)endpoint;
    if (app_result == USB_SERIAL_TX_BUSY) {
        app_result = USB_SERIAL_TX_DONE;
    }
    return USBD_OK;
}

static USBD_CDC_ItfTypeDef interface = {
    .Init = cdc_init,
    .DeInit = cdc_deinit,
    .Control = cdc_control,
    .Receive = cdc_receive,
    .TransmitCplt = cdc_transmit_complete,
};

bool usb_serial_init(void)
{
    return USBD_Init(&device, &usb_descriptors, 0U) == USBD_OK &&
           USBD_RegisterClass(&device, USBD_CDC_CLASS) == USBD_OK &&
           USBD_CDC_RegisterInterface(&device, &interface) == USBD_OK &&
           USBD_Start(&device) == USBD_OK;
}

/* Protege handle e buffer contra reset/configuracao/conclusao pelo USB.
 * SysTick continua habilitado. Nunca executar estas APIs dentro de uma ISR. */
static uint32_t lock_usb(void)
{
    uint32_t enabled = NVIC_GetEnableIRQ(OTG_FS_IRQn);
    NVIC_DisableIRQ(OTG_FS_IRQn);
    return enabled;
}

static void unlock_usb(uint32_t enabled)
{
    if (enabled != 0U) {
        NVIC_EnableIRQ(OTG_FS_IRQn);
    }
}

static bool write_locked(const void *data, uint16_t length)
{
    USBD_CDC_HandleTypeDef *cdc = device.pClassData;
    if (!terminal_open || device.dev_state != USBD_STATE_CONFIGURED ||
        cdc == NULL || cdc->TxState != 0U) {
        return false;
    }
    /* O chamador pode reutilizar seu buffer assim que esta copia terminar. */
    memcpy(tx_buffer, data, length);
    USBD_CDC_SetTxBuffer(&device, tx_buffer, length);
    return USBD_CDC_TransmitPacket(&device) == USBD_OK;
}

bool usb_serial_write(const void *data, uint16_t length)
{
    if (data == NULL || length == 0U || length > sizeof(tx_buffer)) {
        return false;
    }
    uint32_t enabled = lock_usb();
    /* A mensagem inicial sempre precede as futuras mensagens da aplicacao. */
    bool sent = app_result == USB_SERIAL_TX_NONE &&
                !greeting_pending && write_locked(data, length);
    if (sent) {
        app_result = USB_SERIAL_TX_BUSY;
    }
    unlock_usb(enabled);
    return sent;
}

usb_serial_tx_result_t usb_serial_take_result(void)
{
    uint32_t enabled = lock_usb();
    usb_serial_tx_result_t result = app_result;
    if (result == USB_SERIAL_TX_DONE || result == USB_SERIAL_TX_ABORTED) {
        app_result = USB_SERIAL_TX_NONE;
    }
    unlock_usb(enabled);
    return result;
}

void usb_serial_poll(void)
{
    static const char greeting[] =
        "Questao (b): base pronta.\r\n"
        "Etapa 1: LED apagado; medicao ainda nao habilitada.\r\n";
    uint32_t enabled = lock_usb();
    if (greeting_pending && write_locked(greeting, sizeof(greeting) - 1U)) {
        greeting_pending = false;
    }
    unlock_usb(enabled);
}
