// mainsFreq for raspberry pico V1.52 240526 qrt@qland.de 
//
// versions 
// 1.5      initial
// 1.52     revised countFails() in view.cpp

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/adc.h"
#include "time.h"

#include "epaper.h"
#include "draw.h"
#include "text.h"
#include "Dcf.h"
#include "sample.h"
#include "view.h"
#include "Key.h"

#define MULTICORE   1                                   // 0 off, 1 on 
#define BOOTKEY     1                                   // 0 do not wait for key after boot, 1 wait for key after boot

#define NSIZEX  128L                                    // native display size x    display is natively portrait
#define NSIZEY  296L                                    //                     y
#define ISIZE   NSIZEX * NSIZEY / 8                     // image size in bytes

#define PICO_FIRST_ADC_PIN      26                      // first ADC pin

void checkPower();
void prepDisplay();
void core1_entry();

bool service(repeating_timer_t *mst);
void key0_short();
void key1_short();
void key0_long();
void key1_long();
void key0_double();
void key1_double();

void display();

uint8_t image[ISIZE];                                   // allocate image memory
bool direrq;											// display refresh request

#define NUMKEYS		    2	                            // number of keys
#define KEY0_PIN        12                              // left  key pin
#define KEY1_PIN        13                              // right

#define SYS_LED_PIN     10                              // system LED pin

Key keys[NUMKEYS];

int main()
{
    stdio_init_all();

    gpio_init(SYS_LED_PIN);                             // init system LED
    gpio_set_dir(SYS_LED_PIN, GPIO_OUT);                //
    gpio_put(SYS_LED_PIN, 0);                           // LED off

	keys[0].init(KEY0_PIN, &key0_short, &key0_long, &key0_double);  // init keys
	keys[1].init(KEY1_PIN, &key1_short, &key1_long, &key1_double);	//     

    static repeating_timer_t mst;                       // add 20 ms service timer
    add_repeating_timer_ms(20, service, NULL, &mst);    //

    #if BOOTKEY
        keys[0].setOnce(true);                          // enable once
        keys[1].setOnce(true);                          //
        bool once = true;                          

        while(once){                                    // wait for any key press
            for(uint8_t i=0; i<NUMKEYS; i++){  
                if(keys[i].getOnce() == false){
                    once = false;
                    break;
                }
            }

            gpio_put(SYS_LED_PIN, !gpio_get(SYS_LED_PIN));
            sleep_ms(100);
        }

        keys[0].setOnce(false);                         // disable once
        keys[1].setOnce(false);                         //

        gpio_put(SYS_LED_PIN, 0);                       // LED off
    #endif

    #if MULTICORE == 1
        multicore_launch_core1(core1_entry);
    #endif

    adc_init();                                             // ADC init
    adc_gpio_init(PICO_VSYS_PIN);                           //          pin
    adc_select_input(PICO_VSYS_PIN - PICO_FIRST_ADC_PIN);   //     select input   

    EPaper::init(LUT_FULL);                             // init full update
    
    EPaper::clearFrameMemory(0xff);                     // clear ePaper memory, 0=black, 1=white
    EPaper::displayFrame(POSTWAIT);                     //

    EPaper::clearFrameMemory(0xff);                     // both alternating buffers
    EPaper::displayFrame(POSTWAIT);                     //

    Draw::clear(true);                                  // clear display memory, block
	Dcf::receive(false);								// start DCF receiver, do not wait
    Sample::init();                                     // init sample collector

    // main loop - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	uint32_t secs = 0;									// prepare wait for DCF valid

    while(true){                                        // endless loop
        if(Dcf::secs != secs){                          // wait for next second
	        secs = Dcf::secs;                           // prepare next wait
            Sample::store(Dcf::secs);                   // store sample and timestamp

            checkPower();                               // check if power is sufficient
            prepDisplay();                              // prepare display   

            #if MULTICORE == 0
                display();
            #endif
        }
    }
}

// -----------------------------------------------------------------------------

void checkPower()
{
    float u_sys = adc_read() * 9.9f / (1 << 12);                                        // calc system voltage

    if(u_sys < 2.0f){                                                                   // if low voltage
        gpio_set_irq_enabled(DCF_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);  // DCF IRQ off
        gpio_put(DCF_LED_PIN, DCF_LED_OFF);                                             //     LED off

        while(true){                                                                    // toggle LED until capacitors are depleted
            gpio_put(SYS_LED_PIN, !gpio_get(SYS_LED_PIN));                              //
            sleep_ms(50);                                                               //
        }
    }
}

// -----------------------------------------------------------------------------

void prepDisplay()
{
    View::draw();
    direrq = true;
}

// -----------------------------------------------------------------------------

bool service(repeating_timer_t *mst)
{
	for(uint8_t i=0; i<NUMKEYS; i++)		            // check keys
		keys[i].check();

    return true;
}

void key0_short()
{
    View::prevPage();
}

void key1_short()
{
    View::nextPage();
}

void key0_double()
{
    View::firstPage();
}

void key1_double()
{
    View::lastPage();
}

void key0_long()
{
    View::prevView();
}

void key1_long()
{
    View::nextView();
}

// -----------------------------------------------------------------------------
// after transfering code with pico probe a manual reset is necessary
//
void core1_entry()
{
    uint8_t fuc;                                                // full refresh counter

    while(true){                                                // endless loop
        if(direrq){                                             // wait for display request
            while(!EPaper::isSleeping && EPaper::isBusy())      // wait while display is wake and busy
                tight_loop_contents();                          //

            gpio_put(SYS_LED_PIN, 1);                           // system LED on

            if(EPaper::isSleeping){                             // if display is sleeping
                if(++fuc)                                       // full refresh every 256 times
                    EPaper::init(LUT_PARTIAL);                  // wake up display with partial refresh
                else                                            //
                    EPaper::init(LUT_FULL);                     // wake up display with full refresh
            }

            gpio_put(SYS_LED_PIN, 0);                           // system LED off

            EPaper::setFrameMemory(image);                      // send image to ePaper RAM (blocking)
            EPaper::displayFrame(POSTWAIT);                     // display image
            EPaper::sleep();                                    // sleep

            Draw::clear(true);                                  // pre clear image, block
            Draw::line(0, GRAZE, DWIDTH-1, GRAZE, PAT21);       //     draw zero line

            direrq = false;                                     // display done
        }
    }
}

// -----------------------------------------------------------------------------

#if MULTICORE == 0
    void display()
    {
        static uint8_t fuc;

        if(!EPaper::isSleeping && EPaper::isBusy())
            return;

        gpio_put(SYS_LED_PIN, 1);

        if(EPaper::isSleeping){
            if(++fuc)
                EPaper::init(LUT_PARTIAL);
            else
                EPaper::init(LUT_FULL);
        }

        gpio_put(SYS_LED_PIN, 0);

        EPaper::setFrameMemory(image);                      // send image to ePaper RAM (blocking)
        EPaper::displayFrame(POSTWAIT);                     // display image
        EPaper::sleep();                                    // sleep

        Draw::clear(true);                                  // pre clear image, block
        Draw::line(0, GRAZE, DWIDTH-1, GRAZE, PAT21);       //     draw zero line
    }
#endif

// -----------------------------------------------------------------------------

// gpio_put(SYS_LED_PIN, !gpio_get(SYS_LED_PIN));

// uint32_t t;
// t = time_us_32();
// printf("-> %lu\n", time_us_32() - t);
