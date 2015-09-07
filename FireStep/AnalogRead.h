#ifndef ANALOGREAD_H
#define ANALOGREAD_H

#include "Arduino.h"

#define ARDUINO_MEGA

#ifndef PRR
#define PRR PRR0
#endif
// gnarly
#define adc_DUMP() {DEBUG_HEX("ADC",PRR);DEBUG_HEX("A",ADCSRA);DEBUG_HEX("B",ADCSRB);DEBUG_HEX("D",DIDR0);DEBUG_EOL();}
#define adc_ENABLE() {PRR &= ~(1<<PRADC); }
#define adc_DIGITAL_DISABLE(pin) {DIDR0 = (1 << pin); }
#define adc_128KHZ() {ADCSRA |= (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0); }
#define adc_250KHZ() {ADCSRA |= (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (0<<ADPS0); }
#define adc_1MHZ()	 {ADCSRA |= (1<<ADEN) | (1<<ADPS2) | (0<<ADPS1) | (0<<ADPS0); }
#define adc_MUX(pin, left) {ADMUX = (1<<REFS0) | (left<<ADLAR) | (pin & 0x3F); }
#define adc_READY ((ADCSRA & (1<<ADIF)) != 0)
#define adc_INIT(pin, left) {adc_ENABLE(); adc_DIGITAL_DISABLE(pin); adc_1MHZ(); adc_MUX(pin,left); }

// useful
#define ADC_INIT(pin, left) {adc_INIT(pin,left); adc_DUMP();}
#define ADC_LISTEN8(pin) {adc_INIT(pin,1); ADCSRB = 0; ADCSRA |= (1 << ADATE); ADC_SAMPLE(); adc_DUMP();}
#define ADC_LISTEN10(pin) {adc_INIT(pin,0); ADCSRB = 0; ADCSRA |= (1 << ADATE); ADC_SAMPLE(); adc_DUMP();}
#define ADC_SAMPLE() {ADCSRA |= (1<<ADSC);}
#define ADC_READNOW(adch,adcl) {adcl=ADCL; adch=ADCH;}
#define ADC_READ(idle, adch,adcl) {for(idle = 0; !adc_READY; idle++){}	adcl=ADCL; adch=ADCH;}

#endif
