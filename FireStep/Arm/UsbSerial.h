/* UsbSerial.h - Part of FireStep
 *
 * Copyright (C) 2015 Paul Jones
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef USBSERIAL_H_
#define USBSERIAL_H_

#include "Print.h"

class UsbSerial: public Print
{
	public:
		void begin (uint32_t);
		int available (void);
		int read (void);
		size_t write (uint8_t);
};

#endif /* USBSERIAL_H_ */
