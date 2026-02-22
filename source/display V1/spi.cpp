#include "spi.h"
#include "hardware/spi.h"

uint Spi::dma_spi_tx_chan = -1;
dma_channel_config Spi::dma_spi_tx_conf;

void Spi::init()
{    
    spi_init(EP_SPI_PORT, 2000000);			            // baudrate
	gpio_set_function(EP_CS_PIN, GPIO_FUNC_SPI);	    // chip select 				
    gpio_set_function(EP_CLK_PIN, GPIO_FUNC_SPI);       // clock
    gpio_set_function(EP_DIN_PIN, GPIO_FUNC_SPI);       // data
	gpio_put(EP_CS_PIN, HIGH);                          // unselect = HIGH

    if(dma_spi_tx_chan == -1)
        initDma();
}

void Spi::deInit()
{
    spi_deinit(EP_SPI_PORT);

	gpio_set_dir(EP_CS_PIN, GPIO_IN);                   // prevent parasitic supply
	gpio_set_dir(EP_CLK_PIN, GPIO_IN);                  // 
	gpio_set_dir(EP_DIN_PIN, GPIO_IN);                  //
}

void Spi::initDma()
{
    dma_spi_tx_chan = dma_claim_unused_channel(true);
    dma_spi_tx_conf = dma_channel_get_default_config(dma_spi_tx_chan);          

    channel_config_set_transfer_data_size(&dma_spi_tx_conf, DMA_SIZE_8);
    channel_config_set_dreq(&dma_spi_tx_conf, spi_get_dreq(EP_SPI_PORT, true));

    channel_config_set_write_increment(&dma_spi_tx_conf, false);
}

void Spi::sendByte(uint8_t data) 
{
    gpio_put(EP_CS_PIN, LOW);
    spi_write_blocking(EP_SPI_PORT, &data, 1);
	gpio_put(EP_CS_PIN, HIGH);
}

uint8_t repData;

void Spi::sendRepeat(uint8_t data, uint16_t size, bool block)
{
    gpio_put(EP_CS_PIN, LOW);

    repData = data;
    channel_config_set_read_increment(&dma_spi_tx_conf, false);

    dma_channel_configure(dma_spi_tx_chan, &dma_spi_tx_conf,
                          &spi_get_hw(EP_SPI_PORT)->dr,     // write address
                          &repData,                         // read address
                          size,                             // element count
                          true);                            // start

    if(block)
        waitDmaReady();
}

void Spi::sendBuffer(const uint8_t* data, uint16_t size, bool block)
{
    channel_config_set_read_increment(&dma_spi_tx_conf, true);

    gpio_put(EP_CS_PIN, LOW);
 
    dma_channel_configure(dma_spi_tx_chan, &dma_spi_tx_conf,
                          &spi_get_hw(spi_default)->dr,     // write address
                          data,                             // read address
                          size,                             // element count
                          true);                            // start
    
    if(block)
        waitDmaReady();
}

void Spi::waitDmaReady()
{
    dma_channel_wait_for_finish_blocking(dma_spi_tx_chan);

    while(spi_is_busy(EP_SPI_PORT))
        tight_loop_contents();

    gpio_put(EP_CS_PIN, HIGH);
}
