#include "usbd_core.h"

static PCD_HandleTypeDef pcd;

static USBD_StatusTypeDef status(HAL_StatusTypeDef result)
{
    if (result == HAL_OK) {
        return USBD_OK;
    }
    return result == HAL_BUSY ? USBD_BUSY : USBD_FAIL;
}

void HAL_PCD_MspInit(PCD_HandleTypeDef *handle)
{
    (void)handle;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    (void)RCC->AHB1ENR;
    /* PA11=D-, PA12=D+, AF10, push-pull, sem pull-up externo. */
    GPIOA->MODER = (GPIOA->MODER & ~((3U << 22) | (3U << 24))) |
                  (2U << 22) | (2U << 24);
    GPIOA->OTYPER &= ~((1U << 11) | (1U << 12));
    GPIOA->OSPEEDR |= (3U << 22) | (3U << 24);
    GPIOA->PUPDR &= ~((3U << 22) | (3U << 24));
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~((15U << 12) | (15U << 16))) |
                   (10U << 12) | (10U << 16);
    RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;
    (void)RCC->AHB2ENR;
    NVIC_SetPriority(OTG_FS_IRQn, 2U); /* SysTick (0) pode interromper o USB. */
}

void HAL_PCD_MspDeInit(PCD_HandleTypeDef *handle)
{
    (void)handle;
    NVIC_DisableIRQ(OTG_FS_IRQn);
    RCC->AHB2ENR &= ~RCC_AHB2ENR_OTGFSEN;
    GPIOA->MODER &= ~((3U << 22) | (3U << 24));
}

USBD_StatusTypeDef USBD_LL_Init(USBD_HandleTypeDef *device)
{
    pcd.Instance = USB_OTG_FS;
    pcd.Init.dev_endpoints = 4U;
    pcd.Init.speed = PCD_SPEED_FULL;
    pcd.Init.dma_enable = DISABLE;
    pcd.Init.phy_itface = PCD_PHY_EMBEDDED;
    pcd.Init.Sof_enable = DISABLE;
    pcd.Init.low_power_enable = DISABLE;
    pcd.Init.lpm_enable = DISABLE;
    /* Alimentacao pelo proprio USB; nao depende de VBUS em PA9. */
    pcd.Init.vbus_sensing_enable = DISABLE;
    pcd.Init.use_dedicated_ep1 = DISABLE;
    pcd.pData = device;
    device->pData = &pcd;
    if (HAL_PCD_Init(&pcd) != HAL_OK ||
        HAL_PCDEx_SetRxFiFo(&pcd, 128U) != HAL_OK ||
        HAL_PCDEx_SetTxFiFo(&pcd, 0U, 64U) != HAL_OK ||
        HAL_PCDEx_SetTxFiFo(&pcd, 1U, 64U) != HAL_OK ||
        HAL_PCDEx_SetTxFiFo(&pcd, 2U, 16U) != HAL_OK) {
        return USBD_FAIL;
    }
    /* 272 palavras utilizadas, dentro das 320 palavras do OTG FS. */
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_DeInit(USBD_HandleTypeDef *device)
{
    return status(HAL_PCD_DeInit(device->pData));
}

USBD_StatusTypeDef USBD_LL_Start(USBD_HandleTypeDef *device)
{
    HAL_StatusTypeDef result = HAL_PCD_Start(device->pData);
    if (result == HAL_OK) {
        NVIC_ClearPendingIRQ(OTG_FS_IRQn);
        NVIC_EnableIRQ(OTG_FS_IRQn);
    }
    return status(result);
}

USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *device)
{
    NVIC_DisableIRQ(OTG_FS_IRQn);
    return status(HAL_PCD_Stop(device->pData));
}

USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef *device, uint8_t address,
                                uint8_t type, uint16_t size)
{
    return status(HAL_PCD_EP_Open(device->pData, address, size, type));
}

USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef *device, uint8_t address)
{
    return status(HAL_PCD_EP_Close(device->pData, address));
}

USBD_StatusTypeDef USBD_LL_FlushEP(USBD_HandleTypeDef *device, uint8_t address)
{
    return status(HAL_PCD_EP_Flush(device->pData, address));
}

USBD_StatusTypeDef USBD_LL_StallEP(USBD_HandleTypeDef *device, uint8_t address)
{
    return status(HAL_PCD_EP_SetStall(device->pData, address));
}

USBD_StatusTypeDef USBD_LL_ClearStallEP(USBD_HandleTypeDef *device, uint8_t address)
{
    return status(HAL_PCD_EP_ClrStall(device->pData, address));
}

uint8_t USBD_LL_IsStallEP(USBD_HandleTypeDef *device, uint8_t address)
{
    PCD_HandleTypeDef *handle = device->pData;
    return (address & 0x80U) ? handle->IN_ep[address & 0x7FU].is_stall :
                              handle->OUT_ep[address & 0x7FU].is_stall;
}

USBD_StatusTypeDef USBD_LL_SetUSBAddress(USBD_HandleTypeDef *device, uint8_t address)
{
    return status(HAL_PCD_SetAddress(device->pData, address));
}

USBD_StatusTypeDef USBD_LL_Transmit(USBD_HandleTypeDef *device, uint8_t address,
                                  uint8_t *buffer, uint32_t size)
{
    return status(HAL_PCD_EP_Transmit(device->pData, address, buffer, size));
}

USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef *device, uint8_t address,
                                        uint8_t *buffer, uint32_t size)
{
    return status(HAL_PCD_EP_Receive(device->pData, address, buffer, size));
}

uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *device, uint8_t address)
{
    return HAL_PCD_EP_GetRxCount(device->pData, address);
}

void USBD_LL_Delay(uint32_t delay)
{
    HAL_Delay(delay);
}

void OTG_FS_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&pcd);
}

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *handle)
{
    USBD_LL_SetupStage(handle->pData, (uint8_t *)handle->Setup);
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *handle, uint8_t endpoint)
{
    USBD_LL_DataOutStage(handle->pData, endpoint, handle->OUT_ep[endpoint].xfer_buff);
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *handle, uint8_t endpoint)
{
    USBD_LL_DataInStage(handle->pData, endpoint, handle->IN_ep[endpoint].xfer_buff);
}

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *handle)
{
    USBD_LL_SetSpeed(handle->pData, USBD_SPEED_FULL);
    USBD_LL_Reset(handle->pData);
}

void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *handle)
{
    USBD_LL_Suspend(handle->pData);
}

void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *handle)
{
    USBD_LL_Resume(handle->pData);
}

void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *handle)
{
    USBD_LL_DevConnected(handle->pData);
}

void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *handle)
{
    USBD_LL_DevDisconnected(handle->pData);
}
