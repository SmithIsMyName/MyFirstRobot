/*
 *      Author: Charles Minchau
 *
 *		Living room obstacle avoiding robot
 *
 *		A ATmega328p polls a HC-SR04 ultrasonic module reading the distance from the front of the robot to an obstacle and
 *		uses this information to control the wheels with pulse width modulation sent to a L293D quadruple half-h driver
 *		setup in bipolar mode.
 *
 *		Pin placement of ATmega328p
 *
 *		L293D Quadruple half-h driver setup for bipolar control:
 *			Right Forwards:		PD6 / OC0A
 *			Left Forwards:		PD5 / OC0B
 *			Right Backwards:	PB3 / OC2A
 *			Left Backwards:		PD3 / OC2B
 *
 *		HC-SR04 Ultrasonic Module
 *			Echo:				PC5
 *			Trig:				PC4
 */
#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

void init();
void triggerUSMod();
void MovementLogic();
void Forward();
void Stop();
void TurnRight();
void TurnLeft();
void Backward();

int 	SENSDISTANCE 	= 50;						// Distance in CM to stop moving forward from object
int 	FORWARDSPEED 	= 255;						// PWM Value 0-255
int 	TURNSPEED		= 150;						// PWM Value 0-255
int 	BACKSPEED		= 200;						// PWM Value 0-255
int 	WAITTIME		= 30;						// WAITTIME * 60ms = Amount of seconds

int 	distance_cm 	= 0;						// Last measured distance
int 	stateTimer		= 0;						// Amount of time in current state, Roughly: Time in seconds = 60ms * stateTimer
int 	changeState		= 0;						// 0: no change needed, 1: forward, 2: right, 3: left, 4: backward.
_Bool	forward			= 0;						// 1 when moving forward
_Bool	stop			= 0;						// 1 when stopped
_Bool	turnRight		= 0;						// 1 when turning right
_Bool	turnLeft		= 0;						// 1 when turning left
_Bool	backward		= 0;						// 1 when moving backward
_Bool 	turnDirection	= 0;						// Allows toggling of turn direction

/*******************************************INITALIZE PORTS, TIMER, AND INTURRUPTS*******************************************/
void init() {
	DDRD |= (1<<DDD5) | (1<<DDD6) | (1<<DDD3);		// PD3, PD6 Output
	DDRB |= (1<<DDB3);								// PB3 Output
	DDRC |= (1<<DDC4);								// PC4 Output
	DDRC &= ~(1<<DDC5);								// PC5 Input to read Echo
	PORTC |= (1<<PORTC5);							// Enable pull up on C5

	PORTC &= ~((1<<PC4) | (1<<PC3) | (1<<PC2));		// Init PC4, PC3, PC2 as low

	PRR &= ~(1<<PRTIM1);					// To activate timer1 module
	TCNT1 = 0;								// Initial timer value
	TCCR1B |= (1<<CS10);					// Timer without prescaller. Since default clock for atmega328p is 1Mhz period is 1uS
	TCCR1B |= (1<<ICES1);					// First capture on rising edge

	OCR0A = 0;								// 0% Duty Cycle
	OCR0B = 0;								// 0% Duty Cycle
	TCCR0A |= (1<<COM0A1) | (1<<COM0B1);	// Set none-inverting mode
	TCCR0A |= (1<< WGM01) | (1<<WGM00);		// Set fast PWM Mode
	TCCR0B |= (1<<CS01);					// Set prescaler to 8 and starts PWM

	OCR2A = 0;								// 0% Duty Cycle
	OCR2B = 0;								// 0% Duty Cycle
	TCCR2A |= (1<<COM2A1) | (1<<COM2B1);	// set none-inverting mode
	TCCR2A |= (1<<WGM21) | (1<<WGM20);		// set fast PWM Mode
	TCCR2B |= (1<<CS21);					// set prescaler to 8 and starts PWM

	PCICR = (1<<PCIE1);						// Enable PCINT[14:8] we use pin C5 which is PCINT13
	PCMSK1 = (1<<PCINT13);					// Enable C5 interrupt
	sei();									// Enable Global Interrupts
}
/*******************************************MAIN PROGRAM LOGIC*******************************************/
int main() {
	init();
	while (1) {
		/*Query UltraSonic Module*/
		_delay_ms(60); 							// To allow sufficient time between queries (60ms min)
		triggerUSMod();							// Trigger the UltraSonic Module
		/*Decide what movement state to be in*/
		MovementLogic();

	}
}
/*******************************************TRIGGER ULTRA SONIC MODULE*******************************************/
void triggerUSMod() {
	PORTC |= (1<<PC4);						// Set trigger high
	_delay_us(10);							// for 10uS
	PORTC &= ~(1<<PC4);						// to trigger the ultrasonic module
}
/*******************************************DECIDE MOVEMENT STATE*******************************************/
void MovementLogic() {
	stateTimer = stateTimer + 1;				// Keep track of how long state has been constant. Roughly: Time = stateTimer * 60ms
	/**********Trigger Change in state***************/
	if (distance_cm <= SENSDISTANCE && forward == 1) {							// Turn to avoid obstacle
		if (turnDirection) {
			changeState = 2;													// Right
		} else {
			changeState = 3;													// Left
		}
		turnDirection = !turnDirection;											// Alternate left and right
	}
	if (stateTimer > WAITTIME && (turnRight == 1 || turnLeft == 1)) {			// Robot is stuck
		changeState = 4;														// Backward
	}
	if (distance_cm > SENSDISTANCE && !backward) {
		changeState = 1;														// Forward
	}
	if (backward == 1 && stateTimer > WAITTIME) {
		changeState = 1;														// Forward
	}
	/*********Change state triggered*************/
	if (changeState > 0) {											// Ensures movement state of robot is only updated once when required.
		switch(changeState) {
			case 1:
				Forward();
				break;
			case 2:
				TurnRight();
				break;
			case 3:
				TurnLeft();
				break;
			case 4:
				Backward();
				break;
			default:
				Stop();
		}
		changeState = 0;
		stateTimer = 0;
	}
}
/*******************************************SET STATE TO STOP*******************************************/
void Stop() {
	OCR0A = 0;								// 0% Duty Cycle
	OCR0B = 0;								// 0% Duty Cycle

	OCR2A = 0;						    	// 0% Duty Cycle
	OCR2B = 0;						    	// 0% Duty Cycle

	forward			= 0;
	turnRight		= 0;
	turnLeft		= 0;
	backward		= 0;

	stop			= 1;
}
/*******************************************SET STATE FORWARD MOTION*******************************************/
void Forward() {
	Stop();									// Set all motion output and flags to zero
	OCR0A = FORWARDSPEED;					// Right tire forward
	OCR0B = FORWARDSPEED;					// Left tire forward
	forward = 1;							// Set flag
}
/*******************************************SET STATE TURN RIGHT*******************************************/
void TurnRight() {
	Stop();									// Set all motion output and flags to zero
	OCR0B = TURNSPEED;						// Left tire forward
	turnRight = 1;							// Set flag
}
/*******************************************SET STATE TURN LEFTT*******************************************/
void TurnLeft() {
	Stop();									// Set all motion output and flags to zero
	OCR0A = TURNSPEED;						// Right tire forward
	turnLeft = 1;							// Set flag
}
/*******************************************SET STATE BACKWARDS*******************************************/
void Backward() {
	Stop();									// Set all motion output and flags to zero
	OCR2A = BACKSPEED;						// Right tire backwards
	OCR2B = BACKSPEED;						// Left tire backwards
	backward = 1;							// Set flag
}
/*******************************************INTERRUPT FOR ULTRASONIC MODULE*******************************************/
ISR(PCINT1_vect) {							// Interrupt triggers every change of state
	if (bit_is_set(PINC,PC5)) {				// Checks if echo is high
		TCNT1 = 0;								// Reset Timer
	} else {								// If timer is low
		uint16_t numuS = TCNT1;					// Save Timer value
		uint8_t oldSREG = SREG;					// Backup SREG
		cli();									// Disable Global interrupts
		distance_cm = numuS / 58;				// Calculate distance. Amount of uS echo is high / 58
		SREG = oldSREG;							// Enable interrupts
	}
}
