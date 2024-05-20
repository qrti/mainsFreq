#pragma once

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"

#define EP_SPI_PORT     spi0

#define EP_BUSY_PIN     22          // pico busy            in 
#define EP_RST_PIN      21          //      reset           out
#define EP_DC_PIN       20          //      command/data    out
#define EP_CS_PIN       17          //      chip select     out
#define EP_CLK_PIN      18          //      clock           out
#define EP_DIN_PIN      19          //      data (MOSI)     out
#define EP_MISO_PIN     16          //      unused (MISO)   in

#define LOW             0
#define HIGH            1

class Spi 
{
    public:
        static void init();
        static void deInit();
        static void sendByte(uint8_t data);
        static void sendRepeat(uint8_t data, uint16_t size, bool block);
        static void sendBuffer(const uint8_t* data, uint16_t size, bool block);
        static void waitDmaReady();

    private:
        static void initDma();

        static uint dma_spi_tx_chan;
        static dma_channel_config dma_spi_tx_conf;
};
