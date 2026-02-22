// dcf raspberry pico V1.54 250129 qrt@qland.de

#pragma once

#define DCF_PIN  			14						// DCF input pin
#define DCF_LED_PIN 		11						//     LED 

#define DCF_LOGIC           1						// 0 negative, 1 positive      
#define DCF_LED_LOGIC		1						// 0 negative, 1 positive
#define DCF_PULL_MODE		0  						// 0 no pullup / 1 pullup

#define USE_DATETIME		1						// 0 no, 1 yes

#include <stdlib.h>
#include "pico/stdlib.h"

#if USE_DATETIME
#include "DateTimeDcf.h"
#endif

#if DCF_LED_LOGIC == 0								// negative logic, LED cathode is ground
#define DCF_LED_ON  0								
#define DCF_LED_OFF 1
#elif DCF_LED_LOGIC == 1							// positive logic, LED anode is Vcc	
#define DCF_LED_ON  1								
#define DCF_LED_OFF 0
#endif

#define CEST				0x40					// summertime DST CEST MESZ (Z2=0, Z1=1) 
#define CET					0x80					// wintertime     CET  MEZ	(Z2=1, Z1=0)

#define IGNOPU              10                      // ignore pulses after poweron
#define MSZERO				100						// nominal ms DCF zero
#define MSONE				200						//                one				
#define	MSTOL				40						// DCF signal tolerance

#define SYNCINI             (1<<0)                  // DCF sync init
#define SYNCSEC             (1<<1)                  //          second
#define SYNC59              (1<<2)                  //          second 59
#define SYNCDAT             (1<<3)                  //          data taken
#define SYNCDAV             (1<<4)                  //               verified
#define SYNCDAR             (1<<5)                  //               ready to take
#define SYNCED              (1<<7)                  //          ready

#define SYNCRES              ~(SYNCSEC | SYNC59 | SYNCDAT | SYNCDAV | SYNCDAR)		// sync restart
#define SYNCOKN		(SYNCINI | SYNCSEC | SYNC59 | SYNCDAT)							//      ok not verified
#define SYNCOKV		(SYNCINI | SYNCSEC | SYNC59 | SYNCDAT | SYNCDAV)				//         verified

class Dcf
{
public:
	static uint8_t sec;					// time
	static uint8_t min;
	static uint8_t hou;
	static uint8_t day;
	static uint8_t wday;
	static uint8_t month;
	static uint8_t year;
	static uint8_t cet;
	
	static volatile uint8_t status;		// DCF status
	static volatile uint32_t secs;		// seconds since 01.01.2000 or since first sync
	static volatile uint32_t fist;		// first sync time
	static volatile uint32_t last;		// last sync time

	static void receive(bool);
	static bool valid();
	static uint32_t secsMup(uint8_t);

#if USE_DATETIME
	static DateTimeDcf now();
#endif

private:
	static uint8_t hsec;                // help time       
	static uint8_t hmin;
	static uint8_t hhou;
	static uint8_t hday;
	static uint8_t hwday;
	static uint8_t hmonth;
	static uint8_t hyear;
	static uint8_t hcet;

	static uint8_t bit;
	static uint8_t parity;
	static struct repeating_timer timer;

	static volatile uint32_t dt;

	static bool service(struct repeating_timer *t);
	static void pulse(uint gpio, uint32_t events);
	static void secPulse();
	static void bitPulse();

	static void incTime();
	static uint8_t getData();
	static void takeData();
	static uint8_t verifyData();

#if USE_DATETIME
	static uint32_t calcSecs();
#endif

	static uint8_t decBin(uint8_t b);
	static uint32_t deltaT(bool reset);

	// static uint8_t parity(uint8_t b);
};

//#define BINPAT "%c%c%c%c%c%c%c%c"
//#define TOBIN(b)	(b & 0x80 ? '1' : '0'),	\
//				    (b & 0x40 ? '1' : '0'),	\
//					(b & 0x20 ? '1' : '0'), \ 
//					(b & 0x10 ? '1' : '0'), \
//				    (b & 0x08 ? '1' : '0'), \
//				    (b & 0x04 ? '1' : '0'), \
//				    (b & 0x02 ? '1' : '0'), \
//				    (b & 0x01 ? '1' : '0')

//
// char buff[32];
//
//sprintf(buff, BINPAT" : B %u\n", TOBIN(status), dt);
//Serial.write(buff);
