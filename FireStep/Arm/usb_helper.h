/* usb_helper.h - Part of FireStep
 *
 * Copyright (C) 2015 Paul Jones
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef USB_HELPER_H_
#define USB_HELPER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <usb.h>

extern USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface;
extern USB_ClassInfo_MS_Device_t  Disk_MS_Interface;

void usb_systick();
void usb_init();

#ifdef __cplusplus
}
#endif

#endif /* USB_HELPER_H_ */
