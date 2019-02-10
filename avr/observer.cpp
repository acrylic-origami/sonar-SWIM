#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#define F_CPU 16000000UL
#include <util/delay.h>

#define BUFFER_SIZE 1024

volatile uint16_t EEPROM_idx = 0;
volatile uint8_t ch = 0;
volatile bool eeprom_should_flush = false,
              eeprom_flushed = false;
volatile uint8_t buffer[BUFFER_SIZE] = {0};
volatile uint16_t delay = 0;
int main() {
	DDRB = 0xFF;
	
	////////////////
	DDRD = 0xFF;
	GTCCR = (1 << 7) | // assert TSM
	        0b11; // halt all timers
	        
	ICR1 = 200;
	TCCR1A = (0b1010 << 4) | // compare mode: clear on upcount
	         (0b10); // WGM11-10 for phase-correct with ICR1 TOP
	TCCR1B = (0b10 << 3) | // WGN13-12 for phase-correct with ICR1 TOP
	         0b001; // no prescaler
	TCNT1 = 0;
	OCR1A = 100;
	
	OCR0A = 200;
	TCCR0A = (0b0010 << 4) | // compare mode: clear on upcount
	         (0b01); // WGM01-00 for phase-correct with OCCR2A TOP
	TCCR0B = 1 << 3 | // WGM02 for phase-correct with OCCR2A TOP
	         0b001; // no prescaler
	TCNT0 = 100;
	OCR0B = 100;
	
	ADMUX = (0b01 << 6) | // use AVcc (~Vcc) as the input, to hook our pot to
	        (1 << 5) | // left-adjust
	        0b0; // ADC0 input
	ADCSRA = (1 << 7) | // enable
	         (1 << 6) | // start first conversion
	         // (1 << 5) | // autotrigger
	         (1 << 3) | // enable interrupts
	         (0b100); // sample ADC at 16M / 64 / 13 (number of clock cycles to perform single conversion)
	GTCCR &= 0x7F;
	////////////////
	
	_delay_ms(5000); // wait for cap to settle
	PORTB ^= 0xF0;
	
	ADMUX = (0b01 << 6) | // use AVcc (~Vcc) as the input, to hook our pot to
	        (1 << 5) | // left-adjust
	        0b1; // ADC0 input
	sei();
	ADCSRA = (1 << 7) | // enable
	         (1 << 6) | // start first conversion
	         (1 << 3) | // enable interrupts
	         (0b100); // sample ADC at 16M / 64 / 13 (number of clock cycles to perform single conversion)
	while(true) {
		if(eeprom_should_flush && !eeprom_flushed) {
			eeprom_flushed = true;
			eeprom_write_block((const void*)&buffer, (void*)0, BUFFER_SIZE);
			PORTB ^= 0xF0;
		}
	}
}
ISR(ADC_vect) {
	if(EEPROM_idx < BUFFER_SIZE) {
		if(++delay > 80) {
			buffer[EEPROM_idx++] = ADCH;
			// eeprom_write_byte((uint8_t*)EEPROM_idx, ADCH);
			// ADMUX &= 0xF0;
			// ADMUX ^= 1;
			
			delay = 0;
		}
		ADCSRA |= 1 << 6;
	}
	else {
		eeprom_should_flush = true;
		PORTB ^= 0xF0;
	}
}