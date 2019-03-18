/*
 * test.c
 *
 * Created: 3/14/2019 6:25:19 AM
 * Author : Glenn
 */ 

#include <avr/io.h>
unsigned char button = 0x00;
unsigned char hold = 0x00;
unsigned char tempB = 0x00;

unsigned char buttonToggle() {
	if (button && !hold) {
		hold = 1;
		return 1;
	}
	else if (!button && hold) {
		hold = 0;
		return 0;
	}
	return 0; //(!button && !hold)  and (button && hold)
}

int main(void)
{
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0x00; PORTC = 0xFF;
	
	unsigned char input1, latch1, reset1, input2, latch2, reset2;
	unsigned char register1, register2;
	reset1 = 0x08;
	reset2 = 0x80;
	
    /* Replace with your application code */
    while (1) 
    {
		register1 = input1 | latch1 | reset1;
		register2 = input2 | latch2 | reset2;
		button = (~PINC & 0x01);
		if (buttonToggle()) {
			input1 = 0x01;
			input2 = 0x10;
			latch1 = 0x04;
			latch2 = 0x40;
		}
		else {
			input1 = 0x00;
			input2 = 0;
			latch1 = 0;
			
		}
		
		PORTB = register1 | register2;
    }
}

