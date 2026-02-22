#include "Key.h"

Key::Key()
{
}

void Key::init(uint8_t pin, void (*shortFunc)(), void (*longFunc)(), void (*doubleFunc)())
{
	this->pin = pin;
	this->shortFunc = shortFunc;
	this->longFunc = longFunc;
	this->doubleFunc = doubleFunc;

	once = false;										// deactivate once
	pTime = rTime = rCnt = 0;							// reset timers and counter

	gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
	gpio_pull_up(pin);
}

void Key::check(void)
{
	if(gpio_get(pin) == 0){								// key pressed
		if(pTime < 255)									// advance and restrict press time counter
			pTime++;									//

		if(pTime == CLONG)								// check long press	
			longPress();								//

		rTime = 0;										// reset release time			
	}
	else{												// key released
		if(rTime < 255)									// advance and restrict release time counter
			rTime++;

		if(pTime>CSHORT-1 && pTime<CLONG)				// valid key press?
			rCnt++;										// count key releases
			
		if(rTime>CDOUBLE-1 && rCnt==1){					// short press
			shortPress();								//
			rCnt = 0;									//
		}
		else if(rTime>=CSHORT && rCnt==2){				// double press
			doublePress();								//
			rCnt = 0;									//
		}

		pTime = 0;										// reset press time	
	}
}

void Key::shortPress(void)
{
	if(once == false)
		shortFunc();

	once = false;	
}

void Key::longPress(void)
{
	if(once == false)
		longFunc();

	once = false;	
}

void Key::doublePress(void)
{
	if(once == false)
		doubleFunc();

	once = false;	
}
