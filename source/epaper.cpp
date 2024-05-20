#include "epaper.h"

bool EPaper::sleeping;

void EPaper::init(bool lut) 
{
    Spi::init();
    
    gpio_init(EP_RST_PIN);                              // reset
    gpio_set_dir(EP_RST_PIN, GPIO_OUT);                 //
    gpio_put(EP_RST_PIN, HIGH);                         // reset off

    gpio_init(EP_DC_PIN);                               // command / data, LOW / HIGH
    gpio_set_dir(EP_DC_PIN, GPIO_OUT);                  //     

    gpio_init(EP_BUSY_PIN);                             // busy
    gpio_set_dir(EP_BUSY_PIN, GPIO_IN);                 //

	reset();

    sendCmd(DRIVER_OUTPUT_CONTROL);
    sendDataByte((EPD_HEIGHT - 1) & 0xff);
    sendDataByte(((EPD_HEIGHT - 1) >> 8) & 0xff);
    sendDataByte(0x00);                             // GD = 0; SM = 0; TB = 0;

    sendCmd(BOOSTER_SOFT_START_CONTROL);
    sendDataByte(0xd7);
    sendDataByte(0xd6);
    sendDataByte(0x9d);

    sendCmd(WRITE_VCOM_REGISTER);
    sendDataByte(0xa8);                             // VCOM 7C

    sendCmd(SET_DUMMY_LINE_PERIOD);
    sendDataByte(0x1a);                             // 4 dummy lines per gate

    sendCmd(SET_GATE_TIME);
    sendDataByte(0x08);                             // 2 us per line

    sendCmd(BORDER_WAVEFORM_CONTROL);
    sendDataByte(0x03);

    sendCmd(DATA_ENTRY_MODE_SETTING);
    sendDataByte(0x03);                             // x increment, y increment

    setLut(lut);
    sleeping = false;
}

void EPaper::sendCmd(uint8_t cmd) 
{
	gpio_put(EP_DC_PIN, LOW);
    Spi::sendByte(cmd);    
}

void EPaper::sendDataByte(uint8_t data) 
{
    gpio_put(EP_DC_PIN, HIGH);
	Spi::sendByte(data);
}

void EPaper::sendDataRepeat(uint8_t data, uint16_t size, bool block) 
{
    gpio_put(EP_DC_PIN, HIGH);
	Spi::sendRepeat(data, size, block);
}

void EPaper::sendDataBuffer(const uint8_t* data, uint16_t size, bool block) 
{
    gpio_put(EP_DC_PIN, HIGH);
	Spi::sendBuffer(data, size, block);
}

void EPaper::waitIdle() 
{
    while(gpio_get(EP_BUSY_PIN) == HIGH)		// LOW idle, HIGH busy
        sleep_ms(10);							// 100 -> 10
}

void EPaper::reset() 
{
    gpio_put(EP_RST_PIN, LOW);				    // module reset    
    sleep_ms(10);								// 200 -> 10 (datasheet)
    gpio_put(EP_RST_PIN, HIGH);
    sleep_ms(10);								// 200 -> 10 (?)
}

void EPaper::setLut(bool lut) 
{
    const uint8_t *p = lut ? lut_partial_update : lut_full_update;
    sendCmd(WRITE_LUT_REGISTER);

    sendDataBuffer(p, 30, true);

    // for(uint8_t i=0; i<30; i++)
    //     sendData(p[i]);
}

void EPaper::setFrameMemory(const uint8_t* buffer) 
{
    setMemoryArea(0, 0, EPD_WIDTH-1, EPD_HEIGHT-1);
    setMemoryPointer(0, 0);
    sendCmd(WRITE_RAM);    

    sendDataBuffer(buffer, ISIZE_BYTE, true);

    // for(uint16_t i=0; i<ISIZE_BYTE; i++)
    //     sendData(buffer[i]);
}

void EPaper::clearFrameMemory(uint8_t color) 
{
    setMemoryArea(0, 0, EPD_WIDTH-1, EPD_HEIGHT-1);
    setMemoryPointer(0, 0);
    sendCmd(WRITE_RAM);

    sendDataRepeat(color, ISIZE_BYTE, true);

    // for(uint16_t i=0; i<ISIZE_BYTE; i++)
    //     sendData(color);
}

void EPaper::displayFrame(uint8_t wait) 
{
	if(wait == PREWAIT)
		waitIdle();

    sendCmd(DISPLAY_UPDATE_CONTROL_2);
    sendDataByte(0xc4);
    sendCmd(MASTER_ACTIVATION);
    sendCmd(TERMINATE_FRAME_READ_WRITE);

	if(wait == POSTWAIT)
		waitIdle();
}

void EPaper::setMemoryArea(int16_t xs, int16_t ys, int16_t xe, int16_t ye) 
{
    sendCmd(SET_RAM_X_ADDRESS_START_END_POSITION);
    
    sendDataByte((xs >> 3) & 0xff);                     // x point must be the multiple of 8 or the last 3 bits will be ignored 
    sendDataByte((xe >> 3) & 0xff);
    sendCmd(SET_RAM_Y_ADDRESS_START_END_POSITION);
    sendDataByte(ys & 0xff);
    sendDataByte((ys >> 8) & 0xff);
    sendDataByte(ye & 0xff);
    sendDataByte((ye >> 8) & 0xff);
}

void EPaper::setMemoryPointer(int16_t x, int16_t y) 
{
    sendCmd(SET_RAM_X_ADDRESS_COUNTER);
    
    sendDataByte((x >> 3) & 0xff);                      // x point must be the multiple of 8 or the last 3 bits will be ignored 
    sendCmd(SET_RAM_Y_ADDRESS_COUNTER);
    sendDataByte(y & 0xff);
    sendDataByte((y >> 8) & 0xff);
    waitIdle();
}

void EPaper::sleep() 
{
	// datasheet prescribes turning off 
	// oscillator clock and DC/DC & regulator
	// befor entering sleep	
	// but does not tell how to do it

	sendCmd(DEEP_SLEEP_MODE);		
	sendDataByte(0x01);						    // 0x01 = deep sleep mode

	gpio_set_dir(EP_RST_PIN, GPIO_IN);          // prevent parasitic supply
	gpio_set_dir(EP_DC_PIN, GPIO_IN);           //

    Spi::deInit();                              // deInit SPI
    sleeping = true;
}
