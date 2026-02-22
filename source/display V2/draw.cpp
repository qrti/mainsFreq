#include "draw.h"

uint Draw::dma_clear_chan = -1; 
dma_channel_config Draw::dma_clear_conf;
uint8_t Draw::pac;

void Draw::clear(bool block) 
{
    if(dma_clear_chan == -1)
        dmaInit();

    dmaClear(); 

    if(block)       
        dma_channel_wait_for_finish_blocking(dma_clear_chan);   // wait until image is cleared
}

void Draw::dmaInit()
{
    bus_ctrl_hw->priority = BUSCTRL_BUS_PRIORITY_DMA_W_BITS | BUSCTRL_BUS_PRIORITY_DMA_R_BITS;

    dma_clear_chan = dma_claim_unused_channel(true);
    dma_clear_conf = dma_channel_get_default_config(dma_clear_chan);    // default dma transfer data size is 32
    channel_config_set_read_increment(&dma_clear_conf, false);
    channel_config_set_write_increment(&dma_clear_conf, true);
}

uint32_t clear_pattern = 0xffffffff;        // white

void Draw::dmaClear()
{
    dma_channel_configure(dma_clear_chan, &dma_clear_conf,
        image,                              // write pointer
        &clear_pattern,                     // read pointer 
        ISIZE_BYTE / 4,                     // number of transfers 
        true                                // start immediately
    );
}

void Draw::line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t pat)
{
    int16_t ax = 1, ay = 1;
    int16_t dx = x1 - x0;
    int16_t dy = y1 - y0;
    
    if(dx < 0){
        dx = -dx;
        ax = -ax;
    }
    
    if(dy < 0){
        dy = -dy;
        ay = -ay;
    }    
    
    if(dx!=0 && dy!=0){
        if(dx >= dy){
            int16_t r = dx / 2;
            
            for(int16_t x=0; x<dx; x++){
                r += dy;
                
                if(r >= dx){
                    r -= dx;
                    setPixel(x0, y0, pat);
                    x0 += ax;
                    y0 += ay;
                }
                else{
                    setPixel(x0, y0, pat);
                    x0 += ax;
                }
            }
        }
        else{
            int16_t r = dy / 2;
            
            for(int16_t y=0; y<dy; y++){
                r += dx;
                
                if(r >= dy){
                    r -= dy;
                    setPixel(x0, y0, pat);
                    x0 += ax;
                    y0 += ay;                
                }
                else{
                    setPixel(x0, y0, pat);
                    y0 += ay;                
                }
            }            
        }
    }
    else if(dx!=0 && dy==0){
        for(int16_t x=0; x<dx; x++){
            setPixel(x0, y0, pat);
            x0 += ax;
        }
    }
    else if(dx==0 && dy!=0){
        for(int16_t y=0; y<dy; y++){
            setPixel(x0, y0, pat);
            y0 += ay;            
        }
    }    
}

void Draw::setPixel(int16_t x, int16_t y)
{
    if(x<0 || x>DWIDTH-1 || y<0 || y>DHEIGHT-1)
        return;

    *(image + (x << 4) + (DHMAX-y >> 3)) &= ~(0x01 << (y & 0x07));
}

void Draw::setPixel(int16_t x, int16_t y, uint16_t pat) 
{
	static uint8_t pcol;

    if(x<0 || x>DWIDTH-1 || y<0 || y>DHEIGHT-1)
        return;

	if(pcol != pat){
		pac = 0;
		pcol = pat;
	}

    if(pat >= PAT11){
        if(pat == PAT11)
            pat = pac++ & 1 ? COLWHITE : COLBLACK;
        else if(pat == PAT21)
            pat = pac++ & 3 ? COLWHITE : COLBLACK;
        else if(pat == PATDIS)
            pat = (x ^ y) & 4 ? COLBLACK : COLWHITE;
        else if(pat == PATMIS)
            pat = (x + y) % 16 > 3 && (x ^ y) & 1 ? COLBLACK : COLWHITE;
    }

    if(pat == COLBLACK)
        *(image + (x << 4) + (DHMAX-y >> 3)) &= ~(0x01 << (y & 0x07));
    // else
    //     *(image + (x << 4) + (DHMAX-y >> 3)) |= 0x01 << (y & 0x07);
}
