#include "epaper.h"

// bool EPaper::sleeping;

void EPaper::init(const uint8_t *lut) 
{
    initPins();
    
    reset();
    waitIdle();                                         
    sendCmd(SW_RESET);                                  
    waitIdle();                                         

    sendCmd(DRIVER_OUTPUT_CONTROL);
    sendDataByte((EPD_HEIGHT - 1) & 0xff);
    sendDataByte(((EPD_HEIGHT - 1) >> 8) & 0xff);
    sendDataByte(0x00);                             // GD = 0; SM = 0; TB = 0;

    sendCmd(DATA_ENTRY_MODE_SETTING);
    sendDataByte(0x03);                             // x increment, y increment

    setMemoryArea(0, 0, EPD_WIDTH-1, EPD_HEIGHT-1);

    sendCmd(DISPLAY_UPDATE_CONTROL_1);            
    sendDataByte(0x00);
    sendDataByte(0x80); 

    setMemoryPointer(0, 0);
    waitIdle();

    setLutByHost(lut);
}

void EPaper::initFast(const uint8_t *lut) 
{
    reset();
    waitIdle();                                         
    sendCmd(SW_RESET);                                  
    waitIdle();                                         

    sendCmd(DRIVER_OUTPUT_CONTROL);
    sendDataByte((EPD_HEIGHT - 1) & 0xff);
    sendDataByte(((EPD_HEIGHT - 1) >> 8) & 0xff);
    sendDataByte(0x00);                             // GD = 0; SM = 0; TB = 0;

    sendCmd(DATA_ENTRY_MODE_SETTING);
    sendDataByte(0x03);                             // x increment, y increment

    setMemoryArea(0, 0, EPD_WIDTH-1, EPD_HEIGHT-1);

    sendCmd(BORDER_WAVEFORM_CONTROL);       
	sendDataByte(0x05);

    sendCmd(DISPLAY_UPDATE_CONTROL_1);            
    sendDataByte(0x00);
    sendDataByte(0x80);

    setMemoryPointer(0, 0);
    waitIdle();

    setLutByHost(lut);
}

void EPaper::initPins()
{
    Spi::init();                                        // init SPI with pins
    
    gpio_init(EP_RST_PIN);                              // reset pin
    gpio_set_dir(EP_RST_PIN, GPIO_OUT);                 //
    gpio_put(EP_RST_PIN, HIGH);                         // reset off

    gpio_init(EP_DC_PIN);                               // DC pin, command / data, LOW / HIGH
    gpio_set_dir(EP_DC_PIN, GPIO_OUT);                  //     

    gpio_init(EP_BUSY_PIN);                             // busy pin
    gpio_set_dir(EP_BUSY_PIN, GPIO_IN);                 //
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
    while(gpio_get(EP_BUSY_PIN) == HIGH)        // LOW idle, HIGH busy
        sleep_ms(5);                           

    sleep_ms(5);                                
}

void EPaper::reset() 
{
    gpio_put(EP_RST_PIN, LOW);                  // module reset    
    sleep_ms(2);                                
    
    gpio_put(EP_RST_PIN, HIGH);
    sleep_ms(10);                               
}

void EPaper::setLut(const uint8_t *lut) 
{
    sendCmd(WRITE_LUT_REGISTER);
    sendDataBuffer(lut, 153, true);
    waitIdle();
}

void EPaper::setLutByHost(const uint8_t *lut) 
{
    setLut(lut);

    sendCmd(LUT_END_OPTION);                    // (B)
    sendDataByte(*(lut + 153));                 // normal
    
    sendCmd(GATE_VOLTAGE);                   
    sendDataByte(*(lut + 154));                
    
    sendCmd(SOURCE_VOLTAGE);                              
    sendDataByte(*(lut + 155));                 // VSH
    sendDataByte(*(lut + 156));                 // VSH2
    sendDataByte(*(lut + 157));                 // VSL
    
    sendCmd(WRITE_VCOM_REGISTER);               
    sendDataByte(*(lut + 158));                   
}

void EPaper::setFrameMemory(const uint8_t* buffer) 
{
    setMemoryArea(0, 0, EPD_WIDTH-1, EPD_HEIGHT-1);
    setMemoryPointer(0, 0);
    sendCmd(WRITE_RAM);    

    sendDataBuffer(buffer, ISIZE_BYTE, true);
}

void EPaper::setFrameMemoryPartial(const uint8_t* buffer)
{
    gpio_put(EP_RST_PIN, LOW);
    sleep_ms(2);
    gpio_put(EP_RST_PIN, HIGH);
    sleep_ms(2);

    setLut(lut_partial);

	sendCmd(WRITE_REG_DISPLAY_OPT);             // (B)
	sendDataByte(0x00);  
	sendDataByte(0x00);  
	sendDataByte(0x00);  
	sendDataByte(0x00); 
	sendDataByte(0x00);  	
	sendDataByte(0x40);                         // RAM ping-pong enable
	sendDataByte(0x00);  
	sendDataByte(0x00);   
	sendDataByte(0x00);  
	sendDataByte(0x00);

	sendCmd(BORDER_WAVEFORM_CONTROL);       
	sendDataByte(0x80);	

	sendCmd(DISPLAY_UPDATE_CONTROL_2);     
	sendDataByte(0xC0);   

	sendCmd(MASTER_ACTIVATION); 
	waitIdle();  
	
    setMemoryArea(0, 0, EPD_WIDTH-1, EPD_HEIGHT-1);
    setMemoryPointer(0, 0);

    sendCmd(WRITE_RAM);
    sendDataBuffer(buffer, ISIZE_BYTE, true);
}

void EPaper::clearFrameMemory(uint8_t color) 
{
    setMemoryArea(0, 0, EPD_WIDTH-1, EPD_HEIGHT-1);
    setMemoryPointer(0, 0);
    sendCmd(WRITE_RAM);

    sendDataRepeat(color, ISIZE_BYTE, true);
}

void EPaper::displayFrame(uint8_t wait) 
{
    if(wait == PREWAIT)
        waitIdle();

    sendCmd(DISPLAY_UPDATE_CONTROL_2);
    sendDataByte(0xc7);       
    sendCmd(MASTER_ACTIVATION);

    if(wait == POSTWAIT)
        waitIdle();
}

void EPaper::displayFramePartial(uint8_t wait) 
{
    if(wait == PREWAIT)
        waitIdle();

    sendCmd(DISPLAY_UPDATE_CONTROL_2);
    sendDataByte(0x0f);
    sendCmd(MASTER_ACTIVATION);

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

// void EPaper::sleep()
// {
//     // datasheet prescribes turning off 
//     // oscillator clock and DC/DC & regulator
//     // befor entering sleep 
//     // but does not tell how to do it
// 
//     sendCmd(DEEP_SLEEP_MODE);       
//     sendDataByte(0x01);                         // 0x01 = deep sleep mode
// 
//     gpio_set_dir(EP_RST_PIN, GPIO_IN);          // prevent parasitic supply
//     gpio_set_dir(EP_DC_PIN, GPIO_IN);           //
// 
//     Spi::deInit();                              // deInit SPI
//     sleeping = true;
// }
