/* FileSystem.h - Part of FireStep
 *
 * Copyright (C) 2015 Paul Jones
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

void init_eeprom (void);
uint8_t eeprom_read_byte (const uint8_t *address);
void eeprom_write_byte (uint8_t *address, uint8_t data);

#endif /* FILESYSTEM_H_ */
