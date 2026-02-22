#pragma once

#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/structs/bus_ctrl.h"
#include "epaper.h"

// #define DWIDTH      296                     // draw width
// #define DHEIGHT     128                     //      height

// #define DWIDTH      296                         // draw width
// #define DHEIGHT     128                         //      height
// #define DHMAX       (DHEIGHT - 1)
// #define ISIZE_BYTE  ((DWIDTH * DHEIGHT) / 8)

#define COLBLACK    0
#define COLWHITE	1
#define PAT11		2
#define PAT21		3
#define PATDIS      4
#define PATMIS      5

extern uint8_t image[];

class Draw
{
    public:
        static void clear(bool block);
        static void line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t pat);
        static void setPixel(int16_t x, int16_t y);
        static void setPixel(int16_t x, int16_t y, uint16_t pat);

    private:
        static void dmaInit();
        static void dmaClear();

        static uint dma_clear_chan;
        static dma_channel_config dma_clear_conf;

        static uint8_t pac;				// pattern counter
};
