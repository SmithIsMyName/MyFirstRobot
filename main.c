/*
 *      Author: Charles Minchau
 *
 *		PWM:
 *		Right Forwards:		PD6 / OC0A
 *		Left Forwards:		PD5 / OC0B
 *
 *		5V Logic:
 *		Left Backwards:		PC3
 *		Right Backwards:	PC2
 *
 *		Echo:				PC5
 *		Trig:				PC4
 */
#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

void init();
void triggerUSMod();
void DecideState();
void ForwardMotion();
void Stop();
void TurnRight();

int 	SENSDISTANCE 	= 60;						// Distance in CM to stop moving forward from object
int 	FORWARDSPEED 	= 150;						// PWM Value 0-255
int 	TURNSPEED		= 100;						// PWM Value 0-255
int 	distance_cm 	= 0;						// Last measured distance

//char state[10] = "stop";					// { "stop", "forward", "left", "right" }

/*******************************************INITALIZE PORTS, TIMER, AND INTURRUPTS*******************************************/
void init() {
	DDRD |= (1<<DDD5) | (1<<DDD6);					// PD3, PD6 Output
	DDRC |= (1<<DDC4) | (1<<DDC3) | (1<<DDC2);		// PC4, PC3, PC2 Output
	DDRC &= ~(1<<DDC5);								// PC5 Input to read Echo
	PORTC |= (1<<PORTC5);							// Enable pull up on C5

	PORTC &= ~((1<<PC4) | (1<<PC3) | (1<<PC2));		// Init PC4, PC3, PC2 as low
	//Stop();

	PRR &= ~(1<<PRTIM1);					// To activate timer1 module
	TCNT1 = 0;								// Initial timer value
	TCCR1B |= (1<<CS10);					// Timer without prescaller. Since default clock for atmega328p is 1Mhz period is 1uS
	TCCR1B |= (1<<ICES1);					// First capture on rising edge

	OCR0A = 0;								// 0% Duty Cycle
	OCR0B = 0;								// 0% Duty Cycle
	TCCR0A |= (1<<COM0A1) | (1<<COM0B1);	// Set none-inverting mode
	TCCR0A |= (1<< WGM01) | (1<<WGM00);		// Set fast PWM Mode
	TCCR0B |= (1<<CS01);					// Set prescaler to 8 and starts PWM

	PCICR = (1<<PCIE1);						// Enable PCINT[14:8] we use pin C5 which is PCINT13
	PCMSK1 = (1<<PCINT13);					// Enable C5 interrupt
	sei();									// Enable Global Interrupts
}
int main() {
	init();
	while (1) {
		/*Query UltraSonic Module*/
		_delay_ms(60); 							// To allow sufficient time between queries (60ms min)
		triggerUSMod();							// Trigger the UltraSonic Module
		/*Decide what movement state to be in*/
		DecideState();

	}
}
/*******************************************TRIGGER ULTRA SONIC MODULE*******************************************/
void triggerUSMod() {
	PORTC |= (1<<PC4);						// Set trigger high
	_delay_us(10);							// for 10uS
	PORTC &= ~(1<<PC4);						// to trigger the ultrasonic module
}
/*******************************************DECIDE STATE BASED ON DISTANCE*******************************************/
void DecideState() {
	if (distance_cm > SENSDISTANCE) {
		ForwardMotion();
	} else {
		TurnRight();
	}
}
/*******************************************SET STATE FORWARD MOTION*******************************************/
void ForwardMotion() {
	Stop();
	OCR0A = FORWARDSPEED;
	OCR0B = FORWARDSPEED;
}
/*******************************************SET STATE STOP*******************************************/
void Stop() {
	OCR0A = 0;								// 0% Duty Cycle
	OCR0B = 0;								// 0% Duty Cycle
}
/*******************************************SET STATE TURN RIGHT*******************************************/
void TurnRight() {
	Stop();
	OCR0B = FORWARDSPEED;
}
/*******************************************SET STATE TURN LEFTT*******************************************/
void TurnLeft() {
	Stop();
	OCR0A = FORWARDSPEED;
}
/*******************************************INTURRUPT PCINT1 FOR PIN C5*******************************************/
ISR(PCINT1_vect) {
	if (bit_is_set(PINC,PC5)) {					// Checks if echo is high
		TCNT1 = 0;								// Reset Timer
	} else {
		uint16_t numuS = TCNT1;					// Save Timer value
		uint8_t oldSREG = SREG;					// Backup SREG
		cli();									// Disable Global interrupts

		distance_cm = numuS / 58;				// Calculate distance

		SREG = oldSREG;							// Enable interrupts
	}
}
