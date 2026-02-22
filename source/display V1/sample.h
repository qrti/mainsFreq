// https://github.com/GitJer/PwmIn
// https://github.com/GitJer/HC-SR04/tree/main
// https://vanhunteradams.com/Pico/DAC/DMA_DAC.html
// https://github.com/raspberrypi/pico-examples/blob/master/pio/ws2812/ws2812_parallel.c#L252
// https://www.elektronik-kompendium.de/public/schaerer/zerosync.htm

#pragma once

#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "sample.pio.h"
#include "hardware/dma.h"

#define SAMPLE_PIN  			15
#define NUMSAM                  15 * 60 * 60        // in s, max ~ (200 * 1024) / 2 / 3600 ~ 28 h
#define SAMLEATI                3                   // sample lead time in dcf secs

#define ERR_NONE                0<<0                // no error
#define ERR_DIS                 1<<0                // disturbance error
#define ERR_MIS                 1<<1                // missing wave error

typedef struct tSample                      
{                                           
    int8_t sample;
    uint8_t status;
}sSample;

class Sample
{
    public:
        static void init();
        static void store(uint32_t secs);        

        static sSample* samples;            // sample buffer pointer
        static uint32_t csi;                // current sample index
        static uint32_t cst;                // current sample time
        static bool preShift;               // pre shift

    private:
        static void initPio();

        static void initSamples();
        static void initIrq();
        static void pio_ir_handler_RX();
        static void initDma();
        static void shift();

        static void read();

        static PIO pio_sample;   
        static uint sm_sample;    
        static pio_sm_config sm_config_sample;
        static uint sm_offset_sample;

        static uint dma_shift_chan;
        static dma_channel_config dma_shift_conf;

        static uint32_t si;                 // sample index

        static int16_t sum;                 // deviation sum
        static uint16_t cnt;                // cycle counter
        static uint16_t dis;                // disturbance counter
        static uint16_t mis;                // missing wave counter
        
        static volatile int16_t iSum;       // internal deviation sum 
        static volatile uint16_t iCnt;      //          cycle counter
        static volatile uint16_t iDis;      //          disturbance counter
        static volatile uint16_t iMis;      //          missing counter
};
