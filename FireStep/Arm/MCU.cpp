/* MCU.cpp - Part of FireStep
 *
 * Copyright (C) 2015 Paul Jones
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <usb.h>
#include "fireduino.h"

volatile uint32_t us_elapsed;

extern "C"
{

#include "usb_helper.h"
extern void disk_timerproc (void);

void SysTick_Handler (void)
{
	us_elapsed++;
	disk_timerproc();
	usb_systick();
}

void digitalWrite (int16_t pinNum, int16_t value)
{
	uint32_t pin = pinMap[pinNum] & 0x1F;
	uint32_t port = (pinMap[pinNum] >> 5) & 7;

	if (value)
	{
		Chip_GPIO_SetPinOutHigh(LPC_GPIO, port, pin);
	}
	else
	{
		Chip_GPIO_SetPinOutLow(LPC_GPIO, port, pin);
	}
}

void pinMode (uint16_t pinNum, uint16_t mode)
{
	uint32_t pin = pinMap[pinNum] & 0x1F;
	uint32_t port = (pinMap[pinNum] >> 5) & 7;

	switch (mode)
	{
		default:
		case INPUT:
			Chip_IOCON_PinMuxSet(LPC_IOCON, port, pin, IOCON_MODE_INACT | IOCON_FUNC0);
			Chip_GPIO_SetPinDIRInput(LPC_GPIO, port, pin);
			break;

		case INPUT_PULLUP:
			Chip_IOCON_PinMuxSet(LPC_IOCON, port, pin, IOCON_MODE_PULLUP | IOCON_FUNC0);
			Chip_GPIO_SetPinDIRInput(LPC_GPIO, port, pin);
			break;

		case OUTPUT:
			Chip_IOCON_PinMuxSet(LPC_IOCON, port, pin, IOCON_MODE_INACT | IOCON_FUNC0);
			Chip_GPIO_SetPinDIROutput(LPC_GPIO, port, pin);
			break;
	}
}

}
