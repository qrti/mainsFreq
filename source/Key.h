#pragma once

#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"

#define TKCYC	20					// key cycle time in ms
#define CSHORT	(40 / TKCYC)		// cycles short  press
#define CLONG	(2000 / TKCYC)		// 		  long 
#define CDOUBLE	(250 / TKCYC)		//        double 


class Key
{
	public:
		Key();
		void init(uint8_t pin, void (*)(), void (*)(), void (*)());
		void check(void);
		void setOnce(bool once) { this->once = once; };
		bool getOnce() { return once; };

	private:
		void shortPress(void);
		void longPress(void);
		void doublePress(void);

		void (*shortFunc)();
		void (*longFunc)();
		void (*doubleFunc)();

		uint8_t pin;
		uint8_t pTime;
		uint8_t rTime;
		uint8_t rCnt;
		bool once;
};
