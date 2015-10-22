/* fireduino_lpc17xx_40xx_.h - Part of FireStep
 *
 * Copyright (C) 2015 Paul Jones
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef FIREDUINO_LPC_H
#define FIREDUINO_LPC_H

#include <string.h>
#include <stdio.h>
#include <chip.h>
#include "pins.h"

#ifdef __cplusplus  // Don't include C++ stuff when referenced from a C file
#include "ARM/HardwareSerial.h"
#include "ARM/UsbSerial.h"
#include "ARM/FileSystem.h"

extern UsbSerial Serial;
extern HardwareSerial Debug;

#else
#include <stdlib.h>
#endif


#ifdef __cplusplus
extern "C" //--- These parts used by C code ---
{
#endif

extern volatile uint32_t us_elapsed;

inline void delay (uint32_t ms)
{
	uint32_t now = us_elapsed;
	while (us_elapsed < now + ms)
		;
}

void pinMode (uint16_t pinNum, uint16_t mode);
void digitalWrite(int16_t pinNum, int16_t value);

#ifdef __cplusplus
}
#endif    //--- These parts used by C code ---



typedef unsigned char byte;
typedef bool boolean;

#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2


//Disabling interrupts not needed on ARM (and not this simple either...)
#define cli()
#define sei()

#define EEPROM_BYTES 512 /* Actual capacity will be less because eeprom buffer is part of MAX_JSON */
#define EEPROM_END 4096

#define TIMER LPC_TIMER3

//FIXME: 60 cycles @ 120 MHz = 500ns delay. Stupid I know...
#ifndef DELAY500NS
#define DELAY500NS \
		  asm("nop");asm("nop");asm("nop");asm("nop"); asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop"); \
		  asm("nop");asm("nop");asm("nop");asm("nop"); asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop"); \
		  asm("nop");asm("nop");asm("nop");asm("nop"); asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop"); \
		  asm("nop");asm("nop");asm("nop");asm("nop"); asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop"); \
		  asm("nop");asm("nop");asm("nop");asm("nop"); asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop"); \
		  asm("nop");asm("nop");asm("nop");asm("nop"); asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
#endif

// A4983 stepper driver pulse cycle requires 2 microseconds
// DRV8825 stepper driver requires 3.8 microseconds
//   http://www.ti.com/lit/ds/symlink/drv8825.pdf
#define PULSE_WIDTH_DELAY() /* no delay */

#define A4988_PULSE_DELAY 	DELAY500NS;DELAY500NS
#define DRV8825_PULSE_DELAY DELAY500NS;DELAY500NS;DELAY500NS;DELAY500NS
#define STEPPER_PULSE_DELAY DRV8825_PULSE_DELAY

#define HIGH 0x1
#define LOW  0x0

#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2

//#define LED_PIN 13
#define LED_USB 70
#define LED_PIN 71
#define LED_SD  73

typedef enum {
	// LPC Pin Names
	P0_0 = LPC_GPIO0_BASE,
		  P0_1, P0_2, P0_3, P0_4, P0_5, P0_6, P0_7, P0_8, P0_9, P0_10, P0_11, P0_12, P0_13, P0_14, P0_15, P0_16, P0_17, P0_18, P0_19, P0_20, P0_21, P0_22, P0_23, P0_24, P0_25, P0_26, P0_27, P0_28, P0_29, P0_30, P0_31,
	P1_0, P1_1, P1_2, P1_3, P1_4, P1_5, P1_6, P1_7, P1_8, P1_9, P1_10, P1_11, P1_12, P1_13, P1_14, P1_15, P1_16, P1_17, P1_18, P1_19, P1_20, P1_21, P1_22, P1_23, P1_24, P1_25, P1_26, P1_27, P1_28, P1_29, P1_30, P1_31,
	P2_0, P2_1, P2_2, P2_3, P2_4, P2_5, P2_6, P2_7, P2_8, P2_9, P2_10, P2_11, P2_12, P2_13, P2_14, P2_15, P2_16, P2_17, P2_18, P2_19, P2_20, P2_21, P2_22, P2_23, P2_24, P2_25, P2_26, P2_27, P2_28, P2_29, P2_30, P2_31,
	P3_0, P3_1, P3_2, P3_3, P3_4, P3_5, P3_6, P3_7, P3_8, P3_9, P3_10, P3_11, P3_12, P3_13, P3_14, P3_15, P3_16, P3_17, P3_18, P3_19, P3_20, P3_21, P3_22, P3_23, P3_24, P3_25, P3_26, P3_27, P3_28, P3_29, P3_30, P3_31,
	P4_0, P4_1, P4_2, P4_3, P4_4, P4_5, P4_6, P4_7, P4_8, P4_9, P4_10, P4_11, P4_12, P4_13, P4_14, P4_15, P4_16, P4_17, P4_18, P4_19, P4_20, P4_21, P4_22, P4_23, P4_24, P4_25, P4_26, P4_27, P4_28, P4_29, P4_30, P4_31,

	// Not connected
	NC = (int)0xFFFFFFFF
}PinName;

//MotionBoard Arduino -> PinName mapping
//                        0      1      2      3      4      5      6      7      8      9
const PinName pinMap[] = {P0_3 , P0_2 , P2_3 , P2_1 , P2_2 , P2_1 , P2_0 , P0_5 , P0_4 , P4_28, //  0-9
						  P0_0 , P1_28, P4_29, P2_4 , P2_5 , P2_6 , NC   , NC   , P2_7 , P2_8 , // 10-19
						  P0_28, P0_28, P0_19, P0_0 , P0_20, P1_28, P0_21, P1_29, P0_22, NC   , // 20-29
						  P2_11, NC   , P1_29, NC   , P2_12, NC   , P2_13, NC   , P0_11, NC   , // 30-39
						  P2_10, NC   , P0_10, NC   , P0_16, NC   , P0_10, P2_10, P0_1 , P0_1 , // 40-49
						  P0_17, P0_18, P0_15, NC   ,                                           // 50-53
						  P3_26, P0_24, P1_24, P3_25, NC   , NC   , P1_26, P1_27, P0_23, P1_22, // A 0-9
						  P1_23, P1_25, P0_25, P0_26, P1_30, P1_31,                             // A10-15
						  P1_18, P1_19, P1_20, P1_21};                                          // LED1-4


//MotionBoard ADC input map
//FIXME: How does this work??
const uint8_t adcPinMap[] = {2, 3, 4, 5, 6, 7}; //0, 1 are in use as digital IO; 6, 7 are debug serial

//Arduino Analog inputs
static const uint8_t A0 = 54;
static const uint8_t A1 = 55;
static const uint8_t A2 = 56;
static const uint8_t A3 = 57;
static const uint8_t A4 = 58;
static const uint8_t A5 = 59;
static const uint8_t A6 = 60;
static const uint8_t A7 = 61;
static const uint8_t A8 = 62;
static const uint8_t A9 = 63;
static const uint8_t A10 = 64;
static const uint8_t A11 = 65;
static const uint8_t A12 = 66;
static const uint8_t A13 = 67;
static const uint8_t A14 = 68;
static const uint8_t A15 = 69;


#ifdef __cplusplus // Don't include C++ stuff when referenced from a C file
namespace fireduino {
	inline Print& get_Print() {
		return Serial;
	}
	inline int16_t serial_read() {
		return Serial.read();
	}
	inline int16_t serial_available() {
		return Serial.available();
	}
	inline void serial_begin(int32_t baud) {
		Serial.begin(baud);
	}
	inline void serial_print(const char *value) {
		Serial.print(value);
	}
	inline void serial_print(const char value) {
		Serial.print(value);
	}
	inline void serial_print(int16_t value, uint8_t format = DEC) {
		Serial.print(value, format);
	}
	inline void pinMode(uint8_t pinNum, uint16_t mode){
		::pinMode(pinNum, mode);
	}

	inline int16_t digitalRead(uint8_t pinNum) {
		uint32_t pin = pinMap[pinNum] & 0x1F;
		uint32_t port = (pinMap[pinNum] >> 5) & 7;

		return Chip_GPIO_GetPinState(LPC_GPIO, port, pin);

	}
	inline void digitalWrite(uint8_t pinNum, uint8_t value) {
		::digitalWrite(pinNum, value);
	}
	inline void analogWrite(uint8_t dirPin, uint8_t value) {
		//No DAC
	}
	inline int16_t analogRead(uint8_t ch,  bool arduino10bit = true) {
		uint16_t adc = 0;
		if(ch >= sizeof(adcPinMap)) return 0;
		bool ret = Chip_ADC_ReadValue(LPC_ADC, adcPinMap[ch], &adc);
		if(!ret) adc = 0;

		return arduino10bit ? (int)(adc >> 2): (int)adc;
	}
	inline void pulseFast(uint8_t pinNum) { // >=2 microsecond pulse
		digitalWrite(pinNum, HIGH);
		STEPPER_PULSE_DELAY
		;
		digitalWrite(pinNum, LOW);
	}
	inline void delay(uint16_t ms) {
		::delay(ms);
	}
	inline void delayMicroseconds(uint16_t usDelay) {
		//TODO: Use real timer!
		while(--usDelay =! 0)
		{
			DELAY500NS;
			DELAY500NS;
		}
	}
	inline uint8_t eeprom_read_byte(uint8_t *addr) {
		return ::eeprom_read_byte(addr);
	}
	inline void	eeprom_write_byte(uint8_t *addr, uint8_t value) {
		::eeprom_write_byte(addr, value);
	}
	inline uint32_t millis() {
		return us_elapsed;
	}
	inline void delay_stepper_pulse() {
		// Mega is so slow that no delay is needed
	}
	inline uint32_t get_timer64us() {
		return Chip_TIMER_ReadCount(TIMER);
	}
	inline void enable_timer64us(bool enable) {
		if (enable)
			{
				Chip_TIMER_Enable(TIMER);
			}
			else
			{
				Chip_TIMER_Disable(TIMER);
			}
	}
	inline void setup_timer64us() {
		/* Enable timer 0 clock */
		Chip_TIMER_Init(TIMER);

		// Timer setup to mimic 16 bit AVR timer:
		// * 16 Mhz / 1024 prescaler (64 us, 15.625 kHz)
		// * 2^16 * prescaler overflow (4.194304 sec)
		Chip_TIMER_Reset(TIMER);
		Chip_TIMER_PrescaleSet(TIMER, (Chip_Clock_GetPeripheralClockRate(SYSCTL_PCLK_TIMER0) / 15625));
		Chip_TIMER_SetMatch(TIMER, 1, 0xffff);
		Chip_TIMER_ResetOnMatchEnable(TIMER, 1);
	}
	inline void clear_timer64us() {
		//do nothing?
	}
	inline int16_t freeRam () {
		//TODO: Fix this
		return 1000;
	}
} // namespace fireduino
#endif
#endif
