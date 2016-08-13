/*
 * Sample_Code_for_Game_Test.c
 *
 * Created: 4/23/2014 1:17:45 PM
 *  Author: msmith70
 */ 

#define F_CPU 8000000 //cpu frequency
#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>

int main(void)
{	DDRB = 0xFF; // set portB for output
	DDRD = 0x00; // set portD for output
	
	uint8_t i, b0 = 0, b1=0, b2=0, b3=0, data=0xFF, userNum;
	PORTB = 0b11111111;
	
	uint8_t myNum;
	srand(100);
	srand( rand()%16 );
	myNum = rand ()%16;
	
    while(1)
    {
        //TODO:: Please write your application code 
		if ((PIND & _BV(PD0)) == 0) // SW0 is pressed?
		{
			if (b0 == 0)
			{
				b0 = 1; //if a switch is pressed, the input is 0
				data = data & 0b11111110;
				PORTB = data;
				_delay_ms(60); // delay time = 10*8=80ms
			}
			else 
			{
				b0 = 0;
				data = data | 0b00000001;
				PORTB = data;
				_delay_ms(60); // delay time = 10*8=80ms
			}
		}
    }
}