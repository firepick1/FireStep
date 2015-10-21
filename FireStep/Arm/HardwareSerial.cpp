/* HardwareSerial.cpp - Part of FireStep
 *
 * Copyright (C) 2015 Paul Jones
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "fireduino.h"

#define IRQ_SELECTION 	UART0_IRQn
#define HANDLER_NAME 	UART0_IRQHandler

/* Transmit and receive ring buffers */
RINGBUFF_T txring, rxring;

/* Transmit and receive ring buffer sizes */
#define UART_SRB_SIZE 256	/* Send */
#define UART_RRB_SIZE 256	/* Receive */

/* Transmit and receive buffers */
static uint8_t rxbuff[UART_RRB_SIZE], txbuff[UART_SRB_SIZE];

/**
 * @brief	UART 0 interrupt handler using ring buffers
 * @return	Nothing
 */
extern "C" void HANDLER_NAME (void)
{
	Chip_UART_IRQRBHandler(UART_SELECTION, &rxring, &txring);
}

size_t HardwareSerial::write (uint8_t c)
{
	//return Chip_UART_SendBlocking(UART_SELECTION, (char*) (&c), 1);
	return Chip_UART_SendRB(UART_SELECTION, &txring, (char*) (&c), 1);
}

void HardwareSerial::begin (uint32_t baud)
{
	//Setup pinmux
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 2, IOCON_MODE_INACT | IOCON_FUNC1);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 3, IOCON_MODE_INACT | IOCON_FUNC1);

	//Setup UART
	Chip_UART_Init(UART_SELECTION);
	Chip_UART_SetBaud(UART_SELECTION, baud);
	Chip_UART_ConfigData(UART_SELECTION, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(UART_SELECTION, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(UART_SELECTION);

	/* Before using the ring buffers, initialize them using the ring
	 buffer init function */
	RingBuffer_Init(&rxring, rxbuff, 1, UART_RRB_SIZE);
	RingBuffer_Init(&txring, txbuff, 1, UART_SRB_SIZE);

	/* Reset and enable FIFOs, FIFO trigger level 3 (14 chars) */
	Chip_UART_SetupFIFOS(UART_SELECTION, (UART_FCR_FIFO_EN | UART_FCR_RX_RS | UART_FCR_TX_RS | UART_FCR_TRG_LEV3));

	/* Enable receive data and line status interrupt */
	Chip_UART_IntEnable(UART_SELECTION, (UART_IER_RBRINT | UART_IER_RLSINT));

	/* preemption = 1, sub-priority = 1 */
	NVIC_SetPriority(IRQ_SELECTION, 1);
	NVIC_EnableIRQ(IRQ_SELECTION);
}

int HardwareSerial::available (void)
{
	return RingBuffer_GetCount(&rxring);
}

int HardwareSerial::read (void)
{
	uint8_t data = 0;
	uint8_t ret = RingBuffer_Pop(&rxring, &data);

	return ret == 0 ? -1 : data;
}

