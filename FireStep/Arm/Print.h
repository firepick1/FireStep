/* Print.h - Part of FireStep
 *
 * Copyright (C) 2015 Paul Jones
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef LPCOPEN_PRINT_H_
#define LPCOPEN_PRINT_H_

#include "stdint.h"
#include "stddef.h"

class Print
{
public:
	virtual size_t write (uint8_t) = 0;

	size_t print (const char*);
	size_t print (char*);
	size_t print (char);
	size_t print (double, int = 3);
	size_t print (int32_t);
	size_t print (int16_t);

	size_t println ();
	size_t println (char);
	size_t println (char*);
	size_t println (const char*);
};

#endif /* LPCOPEN_PRINT_H_ */
