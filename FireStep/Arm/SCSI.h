/*
 * Copyright(C) NXP Semiconductors, 2012
 * All rights reserved.
 *
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

/** \file
 *
 *  Header file for SCSI.c.
 */

#ifndef _SCSI_H_
#define _SCSI_H_
#include <USB.h>
#include "Descriptors.h"

/* Macros: */
/** Macro to set the current SCSI sense data to the given key, additional sense code and additional sense qualifier. This
 *  is for convenience, as it allows for all three sense values (returned upon request to the host to give information about
 *  the last command failure) in a quick and easy manner.
 *
 *  \param[in] Key    New SCSI sense key to set the sense code to
 *  \param[in] Acode  New SCSI additional sense key to set the additional sense code to
 *  \param[in] Aqual  New SCSI additional sense key qualifier to set the additional sense qualifier code to
 */
#define SCSI_SET_SENSE(Key, Acode, Aqual)  MACROS{ SenseData.SenseKey                 = (Key);   \
		                                                   SenseData.AdditionalSenseCode      = (Acode); \
		                                                   SenseData.AdditionalSenseQualifier = (Aqual); }MACROE

/** Macro for the \ref SCSI_Command_ReadWrite_10() function, to indicate that data is to be read from the storage medium. */
#define DATA_READ           true

/** Macro for the \ref SCSI_Command_ReadWrite_10() function, to indicate that data is to be written to the storage medium. */
#define DATA_WRITE          false

/** Value for the DeviceType entry in the SCSI_Inquiry_Response_t enum, indicating a Block Media device. */
#define DEVICE_TYPE_BLOCK   0x00

/** Value for the DeviceType entry in the SCSI_Inquiry_Response_t enum, indicating a CD-ROM device. */
#define DEVICE_TYPE_CDROM   0x05

/* Function Prototypes: */
bool SCSI_DecodeSCSICommand (USB_ClassInfo_MS_Device_t* const MSInterfaceInfo);

#if defined(INCLUDE_FROM_SCSI_C)
static bool SCSI_Command_Inquiry(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo);
static bool SCSI_Command_Request_Sense(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo);
static bool SCSI_Command_Read_Capacity_10(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo);
static bool SCSI_Command_Send_Diagnostic(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo);
static bool SCSI_Command_ReadWrite_10(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo,
		const bool IsDataRead);
static bool SCSI_Command_ModeSense_6(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo);
#endif

/** Total number of logical drives within the device - must be non-zero. */
#define TOTAL_LUNS                1

/** Total number of bytes of the storage medium, comprised of one or more Dataflash ICs. */
#define VIRTUAL_MEMORY_BYTES                (MassStorage_GetCapacity())

/** Block size of the device. This is kept at 512 to remain compatible with the OS despite the underlying
 *  storage media (Dataflash) using a different native block size. Do not change this value.
 */
#define VIRTUAL_MEMORY_BLOCK_SIZE           512

/** Total number of blocks of the virtual memory for reporting to the host as the device's total capacity. Do not
 *  change this value; change VIRTUAL_MEMORY_BYTES instead to alter the media size.
 */
#define VIRTUAL_MEMORY_BLOCKS               (VIRTUAL_MEMORY_BYTES / VIRTUAL_MEMORY_BLOCK_SIZE)

/** Blocks in each LUN, calculated from the total capacity divided by the total number of Logical Units in the device. */
#define LUN_MEDIA_BLOCKS         (VIRTUAL_MEMORY_BLOCKS / TOTAL_LUNS)

/** Indicates if the disk is write protected or not. */
#define DISK_READ_ONLY            false

void EVENT_USB_Device_Connect (void);
void EVENT_USB_Device_Disconnect (void);
void EVENT_USB_Device_ConfigurationChanged (void);
void EVENT_USB_Device_ControlRequest (void);

bool CALLBACK_MS_Device_SCSICommandReceived (USB_ClassInfo_MS_Device_t* const MSInterfaceInfo);

#endif

