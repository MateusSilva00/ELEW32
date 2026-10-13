#include "usbd_core.h"

/* VID/PID do exemplo CDC da ST, para este experimento com STM32. */
static uint8_t device_descriptor[] = {
    18, USB_DESC_TYPE_DEVICE, 0x00, 0x02, /* USB 2.0 */
    0x02, 0x02, 0x00, USB_MAX_EP0_SIZE, /* CDC ACM */
    0x83, 0x04, 0x40, 0x57,           /* 0483:5740 */
    0x00, 0x01, 1, 2, 3, 1
};
static uint8_t language_descriptor[] = {4, USB_DESC_TYPE_STRING, 0x09, 0x04};
static uint8_t string_descriptor[USBD_MAX_STR_DESC_SIZ];

static uint8_t *device(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    *length = sizeof(device_descriptor);
    return device_descriptor;
}

static uint8_t *language(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    *length = sizeof(language_descriptor);
    return language_descriptor;
}

static uint8_t *string(const char *text, uint16_t *length)
{
    USBD_GetString((uint8_t *)text, string_descriptor, length);
    return string_descriptor;
}

static uint8_t *manufacturer(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    return string("ELEW32", length);
}

static uint8_t *product(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    return string("Black Pill F411 - Questao C", length);
}

static uint8_t *serial(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    static const char hex[] = "0123456789ABCDEF";
    char text[25];
    /* Numero de serie estavel, derivado dos 96 bits do identificador do chip. */
    for (uint32_t word = 0; word < 3U; ++word) {
        uint32_t uid = ((const volatile uint32_t *)UID_BASE)[word];
        for (uint32_t digit = 0; digit < 8U; ++digit) {
            text[word * 8U + digit] = hex[(uid >> (28U - digit * 4U)) & 0xFU];
        }
    }
    text[24] = '\0';
    return string(text, length);
}

static uint8_t *configuration(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    return string("CDC", length);
}

USBD_DescriptorsTypeDef usb_descriptors = {
    .GetDeviceDescriptor = device,
    .GetLangIDStrDescriptor = language,
    .GetManufacturerStrDescriptor = manufacturer,
    .GetProductStrDescriptor = product,
    .GetSerialStrDescriptor = serial,
    .GetConfigurationStrDescriptor = configuration,
    .GetInterfaceStrDescriptor = configuration,
};
