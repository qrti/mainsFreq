// dcf V1.5 240316 qrt@qland.de
//
// V0.1     initial version
// V0.3     automatic sec corrections
// V0.4     refined sec correction
// V0.5     returned to simple sec correction, added lost signal sync
// V0.6     ticker and timer use
// V0.7     simple brightime handling     
// V1.2     receiver noise fix
// V1.3		arduino version
// V1.4		ATMEGA 1284 timer 3 adaption, secs counter
// V1.5     Raspberry Pico version

#include "Dcf.h"

uint8_t Dcf::sec;  
uint8_t Dcf::min;
uint8_t Dcf::hou;
uint8_t Dcf::day;
uint8_t Dcf::wday;
uint8_t Dcf::month;
uint8_t Dcf::year;
uint8_t Dcf::cet;
 
uint8_t Dcf::hsec;  
uint8_t Dcf::hmin;
uint8_t Dcf::hhou;
uint8_t Dcf::hday;
uint8_t Dcf::hwday;
uint8_t Dcf::hmonth;
uint8_t Dcf::hyear;
uint8_t Dcf::hcet;

volatile uint8_t Dcf::status;               
volatile uint32_t Dcf::secs;
volatile uint32_t Dcf::fist;
volatile uint32_t Dcf::last;
volatile uint32_t Dcf::dt;

uint8_t Dcf::bit;               
uint8_t Dcf::parity;               

struct repeating_timer Dcf::timer;

void Dcf::receive(bool wait_valid)
{
    gpio_init(DCF_LED_PIN);                                         // DCF LED
    gpio_set_dir(DCF_LED_PIN, GPIO_OUT);                            //
    gpio_put(DCF_LED_PIN, DCF_LED_OFF);                             // OFF

    add_repeating_timer_us(1000000L, service, NULL, &timer);        // 1 s service 
    
    gpio_init(DCF_PIN);                                             // DCF input
    gpio_set_dir(DCF_PIN, GPIO_IN);                                 //                               
    gpio_disable_pulls(DCF_PIN);                                    // default is pull down (50..80K)

    #if DCF_PULL_MODE                                               // pullup
    gpio_pull_up(DCF_PIN);                                          //
    #endif                                                          //

    gpio_set_irq_enabled_with_callback(DCF_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &pulse);

	if(wait_valid)
		while(!valid())
			sleep_ms(500);
}

bool Dcf::valid()
{
	return (status & SYNCOKN) == SYNCOKN;
}

uint32_t Dcf::secsMup(uint8_t m)
{
	return secs + (m - secs % m);
}

bool Dcf::service(struct repeating_timer *t)
{
	incTime();

	if(status&SYNCDAV && min==58 && sec==50){				// no new sync while data not verified                  
		status &= SYNCRES;									// hourly sync, lost signal sync after LOSIGT ms															
        gpio_set_irq_enabled(DCF_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);   // enable gpio irq
	}
	
	if((status & SYNCOKV) == SYNCOKV)
		gpio_put(DCF_LED_PIN, DCF_LED_ON);

    return true;        
}

void Dcf::pulse(uint gpio, uint32_t events)
{	
	uint8_t r = gpio_get(DCF_PIN);

	if(r == DCF_LOGIC)
		secPulse();
	else
		bitPulse();

	if((status & SYNCOKV) != SYNCOKV)
		gpio_put(DCF_LED_PIN, !r);
}

void Dcf::secPulse()
{	
	dt = deltaT(true);										// get time since last sec pulse

    if(dt<950 || dt>=2050){                                 // receiver noise fix
        if(!(status & SYNCDAV))                             // sync in progress?
            status &= SYNCRES;                              // yes, restart sync

        return;                     
    }

    if(!(status & SYNCINI)){                                // ignore first pulses after poweron
        if(++parity == IGNOPU)                              // parity used as init counter
            status |= SYNCINI;      
    }       
    else if(!(status & SYNCSEC)){                           // sec sync
        status |= SYNCSEC;      
    }       
    else if(!(status & SYNC59)){                            // sec 59 sync
        if(dt>1950 && dt<2050){                             // 2000 ms +- 50 ms
            status |= SYNC59;								// sec 59 sync done
            hsec = 0;                                       // prepare data capture
            parity = 0;
        }
    }
    else if((status & (SYNCDAR | SYNCDAT)) == SYNCDAR){     // data ready, but not taken
        cancel_repeating_timer(&timer);                             // new second starts now
        add_repeating_timer_us(1000000L, service, NULL, &timer);    // restart 1 s service 
        
        takeData();                                         // take data
        status &= ~SYNCDAR;									// reset data ready
		status |= SYNCDAT;                                  // data taken
        hsec = 0;                                           // prepare data verify
        parity = 0;
    }
    else if((status & (SYNCDAR | SYNCDAV)) == SYNCDAR){     // data ready, but not verified                          
        if(verifyData()){									// verify data
            status &= SYNCRES;                              // restart sync if error
		}
		else{
			status &= ~SYNCDAR;								// reset data ready
            status |= SYNCDAV | SYNCED;                     // data verified, DCF sync ready
            gpio_set_irq_enabled(DCF_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);      // disable gpio irq
		}
    }
}

void Dcf::bitPulse()
{
    if((status & (SYNC59 | SYNCDAR)) == SYNC59)				// SYNC59 done, data not ready
        if(getData())                                       // get data
            status &= SYNCRES;                              // restart sync if error
}

void Dcf::incTime()
{
	secs++;

    if(++sec < 60) 
        return;

    sec = 0;

    if(++min < 60) 
        return;

    min = 0;

    if(++hou < 24) 
        return;

    hou = 0;

    if(++day<29 ||
       (day==29 && month==2 && year%4==0 && year%400!=0) ||
       (day<31 && (month==4 || month==6 || month==9 || month==11)) ||
       (day<32 && (month==1 || month==3 || month==5 || month==7 || month==8 || month==10 || month==12)))
        return;

    day = 1;

    if(++month < 13) 
        return;

    month = 1;
    year++;
}

//         5          4          3          2          1                P parity, Y year, M month, W weekday, D day
// 876543210 9876543210 9876543210 9876543210 9876543210 9676543210     h hour, m minute, L leap second, S time start 
// PYYYYYYYY MMMMMWWWDD DDDDPhhhhh hPmmmmmmmS LZZEARRRRR RRRRRRRRR0     Z zone, A approching zone change, R reserved
// --------  ---------- ---|------ |-------|1                     0 
//
uint8_t Dcf::getData()
{
    dt = deltaT(false);										// bit length

    if(dt>70 && dt<110)                                     // zero
        bit = 0;
    else if(dt>170 && dt<210)                               // one
        bit = 0x80;
    else                                                    // not valid
        return 0;											// no error, ignore

    if(hsec == 0){                                          // bit 0
        if(bit == 0x80)                                     // always 0
            return 2;             
    }       
	else if(hsec == 17){									// bit 17
		hcet = bit;											// Z1
	}
	else if(hsec == 18){									// bit 18
		hcet = hcet >> 1 | bit;								// Z2

		if(hcet==0x00 || hcet==0xc0)
			return 3;
	}	
    else if(hsec == 20){                                    // bit 20
        if(bit == 0)                                        // always 1
            return 4;             
    }       
    else if(hsec>20 && hsec<29){                            // bits 21-28
        if(hsec < 28){                                      // minute
            hmin = hmin >> 1 | bit;       

            if(hsec==27 && (hmin=decBin(hmin>>1))>59)       
                return 5;    
        }       

        if((parity^=bit) && hsec==28)                       // parity
            return 6;     
    }       
    else if(hsec>28 && hsec<36){                            // bits 29-35
        if(hsec < 35){                                      // hour
            hhou = hhou >> 1 | bit;

            if(hsec==34 && (hhou=decBin(hhou>>2))>24)       
                return 7;     
        }

        if((parity^=bit) && hsec==35)                       // parity 
            return 8;     
    }       
    else if(hsec>35 && hsec<59){                            // bits 36-58
        if(hsec < 42){                                      // day
            hday = hday >> 1 | bit;

            if(hsec==41 && (hday==0 || (hday=decBin(hday>>2))>31))       
                return 9;     
        }
        else if(hsec < 45){                                 // weekday
            hwday = hwday >> 1 | bit;

            if(hsec==44 && (hwday==0 || (hwday>>=5)>7))		// Mo 1 .. Su 7
                return 10;     
        }
        else if(hsec < 50){                                 // month
            hmonth = hmonth >> 1 | bit;

            if(hsec==49 && (hmonth==0 || (hmonth=decBin(hmonth>>3))>12))    
                return 11;     
        }
        else if(hsec < 58){                                 // year
            hyear = hyear >> 1 | bit;

            if(hsec==57 && (hyear=decBin(hyear))>99)       
                return 12;    
        }

        if((parity^=bit) && hsec==58)                       // parity
            return 13;

        if(hsec == 58)                                      
            status |= SYNCDAR;                              // data ready
    }

    hsec++;

    return 0;
}

void Dcf::takeData()
{
    sec = 0;  
    min = hmin;
    hou = hhou;
    day = hday;
    wday = hwday;
    month = hmonth;
    year = hyear;        
	cet = hcet;

	#if USE_DATETIME
	secs = calcSecs();
	#else
	if(!(status & SYNCED)
		secs = 0;
	#endif

    last = secs;

	if(!fist)
		fist = secs;
}

uint8_t Dcf::verifyData()
{    
    return (hou!=hhou || day!=hday || wday!=hwday || month!=hmonth || year!=hyear || cet!=hcet);
}

uint8_t Dcf::decBin(uint8_t b)
{
    return (b >> 4) * 10 + (b & 0x0f);
}

uint32_t Dcf::deltaT(bool reset)
{
	static uint32_t t, lt, r;
	
    t = to_ms_since_boot(get_absolute_time());
	r = t - lt;

	if(reset)
		lt = t;

	return r;
}

#if USE_DATETIME
DateTimeDcf Dcf::now()
{
	return DateTimeDcf((uint16_t)year+2000, month, day, hou, min, sec);
}

uint32_t Dcf::calcSecs()
{
	DateTimeDcf nowa = now();
	return nowa.get();
}
#endif

// uint8_t Dcf::parity(uint8_t b)
// {
//     b ^= b >> 4;
//     b ^= b >> 2;
//     b ^= b >> 1;
    
//     return b & 1;
// }

// uint8_t x = 0;
// uBit.io.P0.setDigitalValue(x++ & 1);
