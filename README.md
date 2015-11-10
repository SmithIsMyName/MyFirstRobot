Author: Charles Minchau

Living room obstacle avoiding robot

A ATmega328p polls a HC-SR04 ultrasonic module reading the distance
from the front of the robot to an obstacle and uses this information
to control the wheels with pulse width modulation sent to a L293D 
quadruple half-h driver setup in bipolar mode.

Pin placement of ATmega328p

L293D Quadruple half-h driver setup for bipolar control:
	Right Forwards:		PD6 / OC0A
	Left Forwards:		PD5 / OC0B
	Right Backwards:	PB3 / OC2A
	Left Backwards:		PD3 / OC2B

HC-SR04 Ultrasonic Module
	Echo:				PC5
	Trig:				PC4

Video at:
https://youtu.be/oiwDPC5ezAc
