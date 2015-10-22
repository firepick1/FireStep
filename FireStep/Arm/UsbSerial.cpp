/* UsbSerial.cpp - Part of FireStep
 *
 * Copyright (C) 2015 Paul Jones
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <string.h>
#include <stdio.h>
#include "UsbSerial.h"
#include "fireduino.h"
#include "usb_helper.h"

extern "C" int _write (int iFileHandle, char *pcBuffer, int iLength)
{
	if (iFileHandle != 1)
	{
		//FIXME: check return value to make sure ring buffer did not overflow
		//Chip_UART_SendRB(UART_SELECTION, &txring, RED, 5);
		//Chip_UART_SendBlocking(LPC_UART0, RED, 5);
		return Chip_UART_SendBlocking(LPC_UART0, pcBuffer, iLength);
	}
	else
	{
		//Chip_UART_SendBlocking(LPC_UART0, GREEN, 5);
		//Chip_UART_SendBlocking(LPC_UART0, pcBuffer, iLength);
		return CDC_Device_SendData(&VirtualSerial_CDC_Interface, pcBuffer, iLength);
	}
}

void UsbSerial::begin (uint32_t baud)
{
}

int UsbSerial::available (void)
{
	return CDC_Device_BytesReceived(&VirtualSerial_CDC_Interface);
}

int UsbSerial::read (void)
{
	int c = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
	//Chip_UART_SendBlocking(LPC_UART0, BLUE, 5);
	//Chip_UART_SendBlocking(LPC_UART0, (char *) (&c), 1);
	return c;
}

size_t UsbSerial::write (uint8_t c)
{
	//Chip_UART_SendBlocking(LPC_UART0, GREEN, 5);
	//Chip_UART_SendBlocking(LPC_UART0, (char *) (&c), 1);
	return CDC_Device_SendData(&VirtualSerial_CDC_Interface, (char *) (&c), 1);
}

