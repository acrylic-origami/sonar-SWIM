#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 16000000UL
#include <util/delay.h>
#define NUM_SIN 32 // pi * (max of 8-bit timer)
#define QUARTER_SIN 8
#define THIRD_SIN 10
#define TWO_THIRD_SIN 21
// #define NUM_LED 16
#define NUM_LED 8
uint8_t SIN[NUM_SIN] = {5, 6, 7, 8, 9, 9, 10, 10, 10, 10, 10, 9, 9, 8, 7, 6, 5, 4, 3, 2, 1, 1, 0, 0, 0, 0, 0, 1, 1, 2, 3, 4};
int8_t BIAS[2] = { 10, 0 }; // relative to 0xFF >> 1
volatile uint16_t j = 0, k = 0;
int main(void) {
	DDRB = 0b11;
	DDRD = 0xF0;
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
	
	sei();
	while(1) {
		// PORTD = 0;
		// PORTD ^= 0b1000 << 4;
		// _delay_ms(50);
		// PORTD ^= 0b0100 << 4;
		// _delay_ms(50);
		// PORTD ^= 0b0100 << 4;
		// _delay_ms(50);
		// PORTD ^= 0b1000 << 4;
		// _delay_ms(50);
		// for(uint8_t i = 0; i < 2*NUM_LED; i++) {
		// 	PORTD ^= 0b0100 << 4; // clear the reading
		// 	_delay_ms(50);
		// 	// for(j = 0; j < 0x04; j++);
		// }
	}
}

volatile int16_t prevs[2] = {0}; // int8_t (for differential running)
ISR(ADC_vect) {
	if(k++ == 0x5F) {
		PORTD &= ~(0b1100 << 4);
		for(uint8_t i = 0; i < (NUM_LED * 2) * 2; i++) {
			PORTD ^= 0b0100 << 4; // clear the reading
		}
		for(int8_t axis = 0; axis < 2; axis++) {
			uint8_t shift = ((PIND >> 2) & 0b11);
			uint8_t left = ((0xFF >> 1) - ((NUM_LED >> 1) << shift));
			int8_t diff = (prevs[axis] - BIAS[axis] - left) >> shift;
			// if(diff <= prevs[axis]) {
			// 	for(uint8_t i = 0; i < 2*(prevs[axis] - diff); i++)
			// 		PORTD ^= 0b0100 << 4;
			// }
			// else {
				for(uint8_t i = 0; i < 2*(NUM_LED + 1); i++) {
					if((i >> 1) == diff)
						PORTD |= 0b1000 << 4;
					else
						PORTD &= 0b01111111;
					
					PORTD ^= 0b0100 << 4;
				}
			// }
			// prevs[axis] = diff;
		}
		k = 0;
	}
	prevs[ADMUX & 1] = ADCH;
	// ADMUX &= 0xF0;
	// ADMUX |= (PINB >> 2) & 1;
	ADMUX ^= 1;
	ADCSRA |= 1 << 6;
}