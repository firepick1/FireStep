/* FileSystem.cpp - Part of FireStep
 *
 * Copyright (C) 2015 Paul Jones
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */


#include "fireduino.h"
#include <ff.h>

FIL file;
FATFS filesystem;

bool eeprom = false;

void put_rc (FRESULT rc);

void init_eeprom (void)
{
	FRESULT err;
	UINT br;

	//Open filesystem
	err = f_mount(&filesystem, "", 0);
	if (err)
	{
		printf("Error opening filesystem, disabling eeprom emulation\r\n");
		put_rc(err);
		return;
	}

	//Open eeprom file
	delay(500);
	do
	{
		err = f_open(&file, "eeprom.bin", FA_READ | FA_WRITE);
	} while (err == FR_NOT_READY);
	switch (err)
	{
		case FR_OK:
			break;

		case FR_NO_FILE:
			//File does not exist, lets create it
			err = f_open(&file, "eeprom.bin", FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
			if (err)
			{
				printf("Error creating eeprom.bin, disabling eeprom emulation\r\n");
				put_rc(err);
				return;
			}

			//Initialize file with zeros
			err = f_lseek(&file, 0);
			if (err)
			{
				put_rc(err);
				return;
			}
			for (uint16_t i = 0; i < EEPROM_BYTES; i++)
			{
				uint8_t z = 0;
				f_write(&file, &z, 1, &br);
				if (br != 1)
				{
					printf("Error zeroing eeprom.bin, disabling eeprom emulation\r\n");
					return;
				}
			}
			break;

		default:
			//Something else went wrong.
			printf("Error opening eeprom.bin, disabling eeprom emulation\r\n");
			put_rc(err);
			return;
			break;
	}

	//Enable eeprom functions
	eeprom = true;
	return;
}

uint8_t eeprom_read_byte (const uint8_t *address)
{
	FRESULT err;
	UINT br;
	uint8_t data[2];

	if (eeprom)
	{
		err = f_lseek(&file, (DWORD) address);
		if (err)
		{
			put_rc(err);
			return 0;
		}

		f_read(&file, data, 1, &br);
		if (br == 1)
			return data[0];
	}

	return 0;
}

void eeprom_write_byte (uint8_t *address, uint8_t data)
{
	FRESULT err;
	UINT br;

	if (eeprom)
	{
		err = f_lseek(&file, (DWORD) address);
		if (err)
		{
			put_rc(err);
			return;
		}

		f_write(&file, &data, 1, &br);

		err = f_sync(&file);
		if (err)
		{
			put_rc(err);
			return;
		}
	}
}

void put_rc (FRESULT rc)
{
	const char
	*str =
			"OK\0" "DISK_ERR\0" "INT_ERR\0" "NOT_READY\0" "NO_FILE\0" "NO_PATH\0"
			"INVALID_NAME\0" "DENIED\0" "EXIST\0" "INVALID_OBJECT\0" "WRITE_PROTECTED\0"
			"INVALID_DRIVE\0" "NOT_ENABLED\0" "NO_FILE_SYSTEM\0" "MKFS_ABORTED\0" "TIMEOUT\0"
			"LOCKED\0" "NOT_ENOUGH_CORE\0" "TOO_MANY_OPEN_FILES\0";
	char i;

	for (i = 0; i != rc && *str; i++)
	{
		while (*str++);
	}

	printf("rc=%u FR_%s\r\n", (UINT) rc, str);
}

DWORD get_fattime (void)
{
	/* Pack date and time into a DWORD variable */
	return	  ((DWORD)(2020 - 1980) << 25)
			| ((DWORD)1 << 21)
			| ((DWORD)1 << 16)
			| ((DWORD)1 << 11)
			| ((DWORD)1 << 5)
			| ((DWORD)1 >> 1);
}
