#include "stm32f4xx_hal.h"
#include "can.h"
#include "led.h"

extern CAN_HandleTypeDef hcan1;
CAN_FilterTypeDef filter;
uint32_t prescaler;
enum can_bus_state bus_state;

void can_init(void) {
    // default to 125 kbit/s
    prescaler = 48;
    // hcan.Instance = CAN;
    bus_state = OFF_BUS; 
}

void can_set_filter(uint32_t id, uint32_t mask) {
    // see page 825 of RM0091 for details on filters
    // set the standard ID part
    filter.FilterIdHigh = (id & 0x7FF) << 5;
    // add the top 5 bits of the extended ID
    filter.FilterIdHigh += (id >> 24) & 0xFFFF;
    // set the low part to the remaining extended ID bits
    filter.FilterIdLow += ((id & 0x1FFFF800) << 3);

    // set the standard ID part
    filter.FilterMaskIdHigh = (mask & 0x7FF) << 5;
    // add the top 5 bits of the extended ID
    filter.FilterMaskIdHigh += (mask >> 24) & 0xFFFF;
    // set the low part to the remaining extended ID bits
    filter.FilterMaskIdLow += ((mask & 0x1FFFF800) << 3);

    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterBank = 0;
    filter.FilterFIFOAssignment = CAN_FIFO0;
    filter.SlaveStartFilterBank = 0;
    filter.FilterActivation = ENABLE;

    if (bus_state == ON_BUS) {
	    HAL_CAN_ConfigFilter(&hcan1, &filter);
    }
}

void can_enable(void) {
    if (bus_state == OFF_BUS) {
	// hcan.Init.Prescaler = prescaler;
	// hcan.Init.Mode = CAN_MODE_NORMAL;
	// hcan.Init.SJW = CAN_SJW_1TQ;
	// hcan.Init.BS1 = CAN_BS1_4TQ;
	// hcan.Init.BS2 = CAN_BS2_3TQ;
	// hcan.Init.TTCM = DISABLE;
	// hcan.Init.ABOM = DISABLE;
	// hcan.Init.AWUM = DISABLE;
	// hcan.Init.NART = DISABLE;
	// hcan.Init.RFLM = DISABLE;
	// hcan.Init.TXFP = DISABLE;
        hcan1.Instance = CAN1;
        hcan1.Init.Prescaler = 3;
        hcan1.Init.Mode = CAN_MODE_NORMAL;
        hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
        hcan1.Init.TimeSeg1 = CAN_BS1_4TQ;
        hcan1.Init.TimeSeg2 = CAN_BS2_3TQ;
        hcan1.Init.TimeTriggeredMode = DISABLE;
        hcan1.Init.AutoBusOff = DISABLE;
        hcan1.Init.AutoWakeUp = DISABLE;
        hcan1.Init.AutoRetransmission = DISABLE;
        hcan1.Init.ReceiveFifoLocked = DISABLE;
        hcan1.Init.TransmitFifoPriority = DISABLE;
        if (HAL_CAN_Init(&hcan1) != HAL_OK)
        {
            Error_Handler();
        }

        hcan.pTxMsg = NULL;
        // HAL_CAN_Init(&hcan1);
        if (HAL_CAN_Init(&hcan1) != HAL_OK) {
            Error_Handler();
        }
        bus_state = ON_BUS;
	    can_set_filter(0, 0);
    }
}

void can_disable(void) {
    if (bus_state == ON_BUS) {
        // do a bxCAN reset (set RESET bit to 1)
        hcan1.Instance->MCR |= CAN_MCR_RESET;
        bus_state = OFF_BUS;
    }
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
}

void can_set_bitrate(enum can_bitrate bitrate) {
    if (bus_state == ON_BUS) {
        // cannot set bitrate while on bus
        return;
    }

    switch (bitrate) {
    case CAN_BITRATE_10K:
	    prescaler = 300;
        break;
    case CAN_BITRATE_20K:
	    prescaler = 150;
        break;
    case CAN_BITRATE_50K:
	    prescaler = 60;
        break;
    case CAN_BITRATE_100K:
        prescaler = 30;
        break;
    case CAN_BITRATE_125K:
        prescaler = 24;
        break;
    case CAN_BITRATE_250K:
        prescaler = 12;
        break;
    case CAN_BITRATE_500K:
        prescaler = 6;
        break;
    case CAN_BITRATE_750K:
        prescaler = 4;
        break;
    case CAN_BITRATE_1000K:
        prescaler = 3;
        break;
    }
}

void can_set_silent(uint8_t silent) {
    if (bus_state == ON_BUS) {
        // cannot set silent mode while on bus
        return;
    }
    if (silent) {
        hcan1.Init.Mode = CAN_MODE_SILENT;
    } else {
        hcan1.Init.Mode = CAN_MODE_NORMAL;
    }
}

uint32_t can_tx(CanTxMsgTypeDef *tx_msg, uint32_t timeout) {
    uint32_t status;

    // transmit can frame
    hcan.pTxMsg = tx_msg;
    status = HAL_CAN_Transmit(&hcan1, timeout);

	led_on();
    return status;
}

uint32_t can_rx(CanRxMsgTypeDef *rx_msg, uint32_t timeout) {
    uint32_t status;

    hcan.pRxMsg = rx_msg;

    status = HAL_CAN_Receive(&hcan1, CAN_FIFO0, timeout);

	led_on();
    return status;
}

uint8_t is_can_msg_pending(uint8_t fifo) {
    if (bus_state == OFF_BUS) {
        return 0;
    }
    return (__HAL_CAN_MSG_PENDING(&hcan, fifo) > 0);
}
