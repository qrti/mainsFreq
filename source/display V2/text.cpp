// text.cpp V0.71 230816 qrt@qland.de
// 
// V0.71 fixed bug with COMPFB 15

#include "text.h"

int16_t Text::xstart, Text::ystart;
uint8_t* Text::p;
uint8_t* Text::fbp;
uint8_t Text::b, Text::rb;
int8_t Text::rc;
uint8_t* Text::font;
uint8_t Text::orix, Text::oriy, Text::origin;

uint16_t Text::drawText(char* text, int16_t x, int16_t y)
{
    char* p = text;
    xstart = x;
    ystart = y;

    while(*p != 0)
        drawChar(*p++);

    return xstart;        
}

uint16_t Text::drawText(char* text, int16_t x, int16_t y, const uint8_t* _font, uint8_t _origin)
{
    setFont(_font, _origin);
    drawText(text, x, y);

    return xstart;
}

uint16_t Text::getTextWidth(char* text, const uint8_t* _font)
{
    setFont(_font);

    char* cp = text;
    uint16_t tWidth = 0;
    uint8_t spacing = 0;

    while(*cp != 0){
        uint8_t* fp = font;                             // font table pointer
        *fp++;                                          // table +0  height (max 127)
        spacing = *fp++;                                //       +1  spacing
        fp += COMPFB;                                   //           first char

        while(*fp != *cp){                              // char +0   code       search char
            if(*fp == 0)                                //      +1   width      width 0 = end of table
                return 0;                               //      +2   data len   char not found
                                                        //      +3.. data
            fp += *(fp + 2) + 3;                        // + data len = next char
        };

        tWidth += *++fp + spacing; 
        cp++;
    }

    return tWidth - spacing;
}

void Text::setFont(const uint8_t* _font)
{
    font = (uint8_t*)_font;
}

void Text::setFont(const uint8_t* _font, uint8_t _origin)
{
    font = (uint8_t*)_font;
    origin = _origin;
}

void Text::drawChar(char c)
{
    p = font;                                       // font table pointer
    int8_t height = *p++;                           // table +0  height (max 127)
    uint8_t spacing = *p++;                         //       +1  spacing
    fbp = p;                                        //       +2  frequent bytes table
    p += COMPFB;                                    //           first char

    while(*p != c){                                 // char +0   code       search char
        if(*p == 0)                                 //      +1   width      width 0 = end of table
            return;                                 //      +2   data len   char not found
                                                    //      +3.. data
        p += *(p + 2) + 3;                          // + data len = next char
    };

    uint8_t width = *++p;                           // char width
    p += 2;                                         //      data start

    if(origin == TOPLEFT){                          // store current char origin
        oriy = 0;
        orix = 0;
    }
    else if(origin == BOTLEFT){
        orix = 0;
        oriy = height;
    }
    else if(origin == BOTRIGHT){
        orix = width;
        oriy = height;
    }
    else if(origin == MIDXY){
        orix = width / 2;                          
        oriy = height / 2;                          
    }    

    int16_t xpos;                                   // start position
    int16_t ypos = ystart;                          //

    b = rb = rc = 0;                                // byte, repeat byte, repeat counter

    while(height > 0){                              // row by row
        xpos = xstart;                              // start row

        for(uint8_t w=0; w<width; w++){             // column by column
            deComp();                               // decompress
            drawByte(xpos++, ypos, b, height>7 ? 8 : height);
        }

        #if(PRECLEAR == false)
        for(uint8_t i=0; i<spacing; i++)            // spacing
            drawByte(xpos++, ypos, 0, height>7 ? 8 : height);
        #endif

        ypos += 8;                                  // next row
        height -= 8;
    }

    xstart += width + spacing;                      // next char
}

#if COMPFB == 15

void Text::deComp()
{
    if(rc == 0){                                    // repeat counter down?
        b = *p++;                                   // get next byte

        if(b < 0xf0){                               // first frequent byte
           rc = b & 0x0f;                           // repeat counter
           b = rb = *(fbp + (b >> 4));              // get byte
        }
        else{
            if(b & 0x08){                           // first raw byte
                rc = -(b & 0x07);                   // repeat counter
                b = *p++;                           // get byte
            }
            else{                                   // first repeat byte
                rc = (b & 0x07) + 1;                // repeat counter
                b = rb = *p++;                      // get byte
            }
        }
    }
    else if(rc < 0){                                // next raw byte
        rc++;                                       // repeat counter ++
        b = *p++;                                   // get byte
    }
    else{                                           // next frequent or repeat byte
        rc--;                                       // repeat counter --
        b = rb;                                     // get byte
    }
}

#elif COMPFB == 31

void Text::deComp()
{
    if(rc == 0){                                    // repeat counter down?
        b = *p++;                                   // get next byte

        if(b < 0xf8){                               // first frequent byte
           rc = b & 0x07;                           // repeat counter
           b = rb = *(fbp + (b >> 3));              // get byte
        }
        else{                                       // first raw byte
            rc = -(b & 0x07);                       // repeat counter
            b = *p++;                               // get byte
        }
    }
    else if(rc < 0){                                // next raw byte
        rc++;                                       // repeat counter ++
        b = *p++;                                   // get byte
    }
    else{                                           // next frequent byte
        rc--;                                       // repeat counter --
        b = rb;                                     // get byte
    }
}

#endif

void Text::drawByte(int16_t x, int16_t y, uint8_t b, uint8_t h)
{
    for(int16_t yy=y; yy<y+h; yy++){
        #if(PRECLEAR == true)
        if(b & 0x01)                                
        #endif
            Draw::setPixel(x-orix, yy-oriy);

        b >>= 1;
    }
}
