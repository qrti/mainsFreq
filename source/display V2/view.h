#pragma once

#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "epaper.h"
#include "draw.h"
#include "text.h"
#include "sample.h"
#include "dcf.h"
#include "DateTimeDcf.h"

#define GRAZE       (DHEIGHT / 2)                   // graph zero line

#define VW          DWIDTH                          // view width
#define SAPP        views[cView].sapp               // samples per pixel
#define MIMO        views[cView].mimo               // full minute modulo

#define PAGSI       (SAPP * VW)                     // page size
#define PAGAD       (PAGSI * 3 / 4)                 // page advance

#define FBW         11                              // font box width
#define FBH         10                              //          height

typedef struct tView                     
{                                           
    uint16_t sapp;                                  // samples per pixel
    uint16_t mimo;                                  // full minute modulo
    char td[6];                                     // time display, incl. termination
}sView;

class View
{
    public:
        static void prevPage();
        static void nextPage();        
        static void firstPage();
        static void lastPage();        
        static void prevView();
        static void nextView();
        static void draw();

    private:
        static void countFails();
        static void drawStatus();
        static void drawTimeStamps();
        static void drawSamples();

        static int8_t getSample(uint16_t si);
        static void timeStamp(uint16_t x, uint32_t t);

        static char buf[];   					// sprintf buffer

        static int8_t cView;                    // current view
        static int32_t vsi;                     // view start index
        static uint8_t lwd;                     // last weekday
        static uint16_t page;                   // page number
        static uint16_t pages;                  // number of pages
        static uint32_t nDis;                   // number of disturbances
        static uint32_t nMis;                   // number of missing waves
};
