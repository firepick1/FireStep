/* usb_helper.c - Part of FireStep
 *
 * Copyright (C) 2015 Paul Jones
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <usb.h>
#include <chip.h>
#include "Descriptors.h"
#include "scsi.h"
#include "fireduino_lpc17xx_40xx.h"

void EVENT_USB_Device_Connect (void);
void EVENT_USB_Device_Disconnect (void);
void EVENT_USB_Device_ConfigurationChanged (void);
void EVENT_USB_Device_ControlRequest (void);

/** LPCUSBlib CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
	{
		.Config =
			{
				.ControlInterfaceNumber         = 0,

				.DataINEndpointNumber           = CDC_TX_EPNUM,
				.DataINEndpointSize             = CDC_TXRX_EPSIZE,
				.DataINEndpointDoubleBank       = false,

				.DataOUTEndpointNumber          = CDC_RX_EPNUM,
				.DataOUTEndpointSize            = CDC_TXRX_EPSIZE,
				.DataOUTEndpointDoubleBank      = false,

				.NotificationEndpointNumber     = CDC_NOTIFICATION_EPNUM,
				.NotificationEndpointSize       = CDC_NOTIFICATION_EPSIZE,
				.NotificationEndpointDoubleBank = false,
			},
	};


/** LPCUSBlib Mass Storage Class driver interface configuration and state information. This structure is
 *  passed to all Mass Storage Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
//USB_ClassInfo_MS_Device_t Disk_MS_Interface =
//{
//	.Config =
//	{
//		.InterfaceNumber           = INTERFACE_ID_MassStorage,
//		.DataINEndpointNumber      = MASS_STORAGE_IN_EPADDR,
//		.DataINEndpointSize        = MASS_STORAGE_IO_EPSIZE,
//		.DataINEndpointDoubleBank  = false,
//
//		.DataOUTEndpointNumber     = MASS_STORAGE_OUT_EPADDR,
//		.DataOUTEndpointSize       = MASS_STORAGE_IO_EPSIZE,
//		.DataOUTEndpointDoubleBank = false,
//
//		.TotalLUNs                 = TOTAL_LUNS,
//	},
//};

void usb_init()
{
	//Setup USB_CONNECT pin
	Chip_IOCON_PinMuxSet(LPC_IOCON, 2, 9, IOCON_MODE_INACT | IOCON_FUNC1);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 2, 9);
	USB_Init();

	//Init LED
	pinMode(LED_USB, OUTPUT);
	digitalWrite(LED_USB, LOW);
}

/* Run this every 1ms to process events */
void usb_systick()
{
	CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
	//MS_Device_USBTask(&Disk_MS_Interface);
	USB_USBTask();
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
	digitalWrite(LED_USB, HIGH);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
	digitalWrite(LED_USB, LOW);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);

//	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}

void EVENT_CDC_Device_LineEncodingChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo)
{
}

/** Mass Storage class driver callback function the reception of SCSI commands from the host, which must be processed.
 *
 *  \param[in] MSInterfaceInfo  Pointer to the Mass Storage class interface configuration structure being referenced
 */
bool CALLBACK_MS_Device_SCSICommandReceived(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo)
{
	bool CommandSuccess;

	//LEDs_SetAllLEDs(LEDMASK_USB_BUSY);
	CommandSuccess = SCSI_DecodeSCSICommand(MSInterfaceInfo);
	//LEDs_SetAllLEDs(LEDMASK_USB_READY);

	return CommandSuccess;
}


