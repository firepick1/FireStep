/* Print.cpp - Part of FireStep
 *
 * Copyright (C) 2015 Paul Jones
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "fireduino.h"
#include "Print.h"

size_t Print::print (const char* in)
{
	return print((char*) in);
}

size_t Print::print (char* in)
{
	char* c = in;
	size_t n = 0;

	while (*c)
		n += write(*c++);

	return n;
}

size_t Print::print (char c)
{
	return write(c);
}

size_t Print::print (double d, int n)
{
	char buf[20];
	snprintf(buf, 20, "%.*f", n, d);
	print(buf);
	return strlen(buf);
}

size_t Print::print (int32_t l)
{
	char buf[20];
	snprintf(buf, 20, "%ld", l);
	print(buf);
	return strlen(buf);
}

size_t Print::print (int16_t i)
{
	char buf[20];
	snprintf(buf, 20, "%d", i);
	print(buf);
	return strlen(buf);
}

size_t Print::println ()
{
	return print("\r\n");
}

size_t Print::println (char c)
{
	return print(c) + println();
}

size_t Print::println (char* c)
{
	return print(c) + println();
}

size_t Print::println (const char* c)
{
	return print(c) + println();
}
