#pragma once

#include <stdlib.h>
#include "pico/stdlib.h"
#include "spi.h"

#define EPD_WIDTH       128                     // display is natively portrait
#define EPD_HEIGHT      296                     //

#define DWIDTH          296                     // draw width
#define DHEIGHT         128                     //      height
#define DHMAX           (DHEIGHT - 1)
#define ISIZE_BYTE      ((DWIDTH * DHEIGHT) / 8)

#define NOWAIT			0
#define PREWAIT			1
#define POSTWAIT		2

#define LOW             0
#define HIGH            1

#define DRIVER_OUTPUT_CONTROL                       0x01
#define BOOSTER_SOFT_START_CONTROL                  0x0c
#define GATE_SCAN_START_POSITION                    0x0f
#define DEEP_SLEEP_MODE                             0x10
#define DATA_ENTRY_MODE_SETTING                     0x11
#define SW_RESET                                    0x12
#define TEMPERATURE_SENSOR_CONTROL                  0x1a
#define MASTER_ACTIVATION                           0x20
#define DISPLAY_UPDATE_CONTROL_1                    0x21
#define DISPLAY_UPDATE_CONTROL_2                    0x22
#define WRITE_RAM                                   0x24
#define WRITE_VCOM_REGISTER                         0x2c
#define WRITE_LUT_REGISTER                          0x32
#define SET_DUMMY_LINE_PERIOD                       0x3a
#define SET_GATE_TIME                               0x3b
#define BORDER_WAVEFORM_CONTROL                     0x3c
#define SET_RAM_X_ADDRESS_START_END_POSITION        0x44
#define SET_RAM_Y_ADDRESS_START_END_POSITION        0x45
#define SET_RAM_X_ADDRESS_COUNTER                   0x4e
#define SET_RAM_Y_ADDRESS_COUNTER                   0x4f
#define TERMINATE_FRAME_READ_WRITE                  0xff

extern uint8_t image[];

#define LUT_FULL        0
#define LUT_PARTIAL     1

const uint8_t lut_full_update[] =
{
    0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22, 
    0x66, 0x69, 0x69, 0x59, 0x58, 0x99, 0x99, 0x88, 
    0x00, 0x00, 0x00, 0x00, 0xF8, 0xB4, 0x13, 0x51, 
    0x35, 0x51, 0x51, 0x19, 0x01, 0x00
};

const uint8_t lut_partial_update[] =
{
    0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x13, 0x14, 0x44, 0x12, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

class EPaper
{
    public:
        static void init(bool lut);
        static void setFrameMemory(const uint8_t* buffer);
        static void clearFrameMemory(uint8_t color);
        static void displayFrame(uint8_t wait);
        static void sleep();
        static bool isBusy() { return gpio_get(EP_BUSY_PIN); };     // LOW: idle, HIGH: busy
        static bool isSleeping() { return sleeping; };

    private:
        static void reset();
        static void waitIdle();
        static void sendCmd(uint8_t cmd);
        static void sendDataByte(uint8_t data);
        static void sendDataRepeat(uint8_t data, uint16_t size, bool block);
        static void sendDataBuffer(const uint8_t* data, uint16_t size, bool block);
        static void setLut(bool lut);
        static void setMemoryArea(int16_t xs, int16_t ys, int16_t xe, int16_t ye);
        static void setMemoryPointer(int16_t x, int16_t y);

        static bool sleeping;
};
