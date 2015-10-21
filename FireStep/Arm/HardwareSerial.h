/* HardwareSerial.h - Part of FireStep
 *
 * Copyright (C) 2015 Paul Jones
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef LPCOPEN_SERIAL_H_
#define LPCOPEN_SERIAL_H_

#include "Print.h"

#define UART_SELECTION 	LPC_UART0

extern RINGBUFF_T txring, rxring;

class HardwareSerial: public Print
{
public:
	void begin (uint32_t);
	int available (void);
	int read (void);
	size_t write (uint8_t);
};

#endif /* LPCOPEN_SERIAL_H_ */
