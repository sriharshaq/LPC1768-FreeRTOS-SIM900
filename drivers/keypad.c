
#include "keypad.h"
#include "LPC17xx.h"
#include "delay.h"
#include "stdint.h"


const uint8_t key_map[] = {	'0','1','2','3',
							'4','5','6','7',
							'8','9','*','#',
							'A','B','C','D'
						  };

uint8_t read_keypad(void)
{
	// Column as input
	LPC_GPIO1->FIODIR &= ~(1 << KEY_C0);
	LPC_GPIO1->FIODIR &= ~(1 << KEY_C1);
	LPC_GPIO1->FIODIR &= ~(1 << KEY_C2);
	LPC_GPIO1->FIODIR &= ~(1 << KEY_C3);
	LPC_GPIO1->FIOSET |= (1 << KEY_C0);
	LPC_GPIO1->FIOSET |= (1 << KEY_C1);
	LPC_GPIO1->FIOSET |= (1 << KEY_C2);
	LPC_GPIO1->FIOSET |= (1 << KEY_C3);

	// Row as output
	LPC_GPIO1->FIODIR |= (1 << KEY_R0);
	LPC_GPIO1->FIODIR |= (1 << KEY_R1);
	LPC_GPIO1->FIODIR |= (1 << KEY_R2);
	LPC_GPIO1->FIODIR |= (1 << KEY_R3);

	// Make R0 as 0
	LPC_GPIO1->FIOCLR |= (1 << KEY_R0);
	LPC_GPIO1->FIOSET |= (1 << KEY_R1);
	LPC_GPIO1->FIOSET |= (1 << KEY_R2);
	LPC_GPIO1->FIOSET |= (1 << KEY_R3);

	if( ((LPC_GPIO1->FIOPIN) & (1 << KEY_C0)) == 0 )
	{
		_delay_ms(_SWITCH_DEB_DELAY);
		while(((LPC_GPIO1->FIOPIN) & (1 << KEY_C0)) == 0);
		return key_map[0];
	}
	if( ((LPC_GPIO1->FIOPIN) & (1 << KEY_C1)) == 0 )
	{
		_delay_ms(_SWITCH_DEB_DELAY);
		while(((LPC_GPIO1->FIOPIN) & (1 << KEY_C1)) == 0);
		return key_map[1];
	}	
	if( ((LPC_GPIO1->FIOPIN) & (1 << KEY_C2)) == 0 )
	{
		_delay_ms(_SWITCH_DEB_DELAY);
		while(((LPC_GPIO1->FIOPIN) & (1 << KEY_C2)) == 0);
		return key_map[2];
	}
	if( ((LPC_GPIO1->FIOPIN) & (1 << KEY_C3)) == 0 )
	{
		_delay_ms(_SWITCH_DEB_DELAY);
		while(((LPC_GPIO1->FIOPIN) & (1 << KEY_C3)) == 0);
		return key_map[3];
	}

	// Make R1 as 0
	LPC_GPIO1->FIOSET |= (1 << KEY_R0);
	LPC_GPIO1->FIOCLR |= (1 << KEY_R1);
	LPC_GPIO1->FIOSET |= (1 << KEY_R2);
	LPC_GPIO1->FIOSET |= (1 << KEY_R3);

	if( ((LPC_GPIO1->FIOPIN) & (1 << KEY_C0)) == 0 )
	{
		_delay_ms(_SWITCH_DEB_DELAY);
		while(((LPC_GPIO1->FIOPIN) & (1 << KEY_C0)) == 0);
		return key_map[4];
	}
	if( ((LPC_GPIO1->FIOPIN) & (1 << KEY_C1)) == 0 )
	{
		_delay_ms(_SWITCH_DEB_DELAY);
		while(((LPC_GPIO1->FIOPIN) & (1 << KEY_C1)) == 0);
		return key_map[5];
	}	
	if( ((LPC_GPIO1->FIOPIN) & (1 << KEY_C2)) == 0 )
	{
		_delay_ms(_SWITCH_DEB_DELAY);
		while(((LPC_GPIO1->FIOPIN) & (1 << KEY_C2)) == 0);
		return key_map[6];
	}
	if( ((LPC_GPIO1->FIOPIN) & (1 << KEY_C3)) == 0 )
	{
		_delay_ms(_SWITCH_DEB_DELAY);
		while(((LPC_GPIO1->FIOPIN) & (1 << KEY_C3)) == 0);
		return key_map[7];
	}

	// Make R2 0
	LPC_GPIO1->FIOSET |= (1 << KEY_R0);
	LPC_GPIO1->FIOSET |= (1 << KEY_R1);
	LPC_GPIO1->FIOCLR |= (1 << KEY_R2);
	LPC_GPIO1->FIOSET |= (1 << KEY_R3);

	if( ((LPC_GPIO1->FIOPIN) & (1 << KEY_C0)) == 0 )
	{
		_delay_ms(_SWITCH_DEB_DELAY);
		while(((LPC_GPIO1->FIOPIN) & (1 << KEY_C0)) == 0);
		return key_map[8];
	}
	if( ((LPC_GPIO1->FIOPIN) & (1 << KEY_C1)) == 0 )
	{
		_delay_ms(_SWITCH_DEB_DELAY);
		while(((LPC_GPIO1->FIOPIN) & (1 << KEY_C1)) == 0);
		return key_map[9];
	}	
	if( ((LPC_GPIO1->FIOPIN) & (1 << KEY_C2)) == 0 )
	{
		_delay_ms(_SWITCH_DEB_DELAY);
		while(((LPC_GPIO1->FIOPIN) & (1 << KEY_C2)) == 0);
		return key_map[10];
	}
	if( ((LPC_GPIO1->FIOPIN) & (1 << KEY_C3)) == 0 )
	{
		_delay_ms(_SWITCH_DEB_DELAY);
		while(((LPC_GPIO1->FIOPIN) & (1 << KEY_C3)) == 0);
		return key_map[11];
	}

	// Make R0 0
	LPC_GPIO1->FIOSET |= (1 << KEY_R0);
	LPC_GPIO1->FIOSET |= (1 << KEY_R1);
	LPC_GPIO1->FIOSET |= (1 << KEY_R2);
	LPC_GPIO1->FIOCLR |= (1 << KEY_R3);

	if( ((LPC_GPIO1->FIOPIN) & (1 << KEY_C0)) == 0 )
	{
		_delay_ms(_SWITCH_DEB_DELAY);
		while(((LPC_GPIO1->FIOPIN) & (1 << KEY_C0)) == 0);
		return key_map[12];
	}
	if( ((LPC_GPIO1->FIOPIN) & (1 << KEY_C1)) == 0 )
	{
		_delay_ms(_SWITCH_DEB_DELAY);
		while(((LPC_GPIO1->FIOPIN) & (1 << KEY_C1)) == 0);
		return key_map[13];
	}	
	if( ((LPC_GPIO1->FIOPIN) & (1 << KEY_C2)) == 0 )
	{
		_delay_ms(_SWITCH_DEB_DELAY);
		while(((LPC_GPIO1->FIOPIN) & (1 << KEY_C2)) == 0);
		return key_map[14];
	}
	if( ((LPC_GPIO1->FIOPIN) & (1 << KEY_C3)) == 0 )
	{
		_delay_ms(_SWITCH_DEB_DELAY);
		while(((LPC_GPIO1->FIOPIN) & (1 << KEY_C3)) == 0);
		return key_map[15];
	}
	return NO_KEY_PRESS;
}