#include "view.h"

int8_t View::cView = 0;                         // current view                          
int32_t View::vsi = -1;                         // view start index off
uint8_t View::lwd;                              // last weekday
uint16_t View::page;
uint16_t View::pages;
uint32_t View::nDis;
uint32_t View::nMis;

char View::buf[32];   		                    // sprintf buffer               

#define NUMVIEWS    5                           // number of views

sView views[] = {                               // views 
    {   1,   1 * 60, "5m" },                    // td = VW * spp / 60       [m]          
    {   5,   5 * 60, "25m" },                   //       
    {  15,  15 * 60, "1.25h" },                 // 
    {  30,  30 * 60, "2.5h" },                  //      VW * spp / 60 / 60  [h]       
    { 180, 180 * 60, "15h" }                    //      
};

uint32_t ts, te;                                // start, end time
int32_t is, ie;                                 //            index 
uint8_t status;                                 // sample status

void View::draw()
{
    if(vsi == -1){                              // show latest samples
        is = 0;                                 // num samples < page size
        ie = Sample::csi;                       //
        ts = Sample::cst - Sample::csi;         //
        te = Sample::cst;                       //

        if(Sample::csi >= PAGSI){               // num samples >= page size       
            is = ie - PAGSI;                    //
            ts = te - PAGSI;                    //
        }
    }
    else{                                       // show page
        is = vsi;                               // start index
        ie = vsi + PAGSI;                       // end

        if(ie > Sample::csi){                   // restrict end index
            ie = Sample::csi;                   //
            is = ie - PAGSI;                    //
            if(is < 0) is = 0;                  //
        }

        ts = Sample::cst - Sample::csi + is;    // start time
        te = Sample::cst - Sample::csi + ie;    // end
    }

    uint16_t os = ts % SAPP ? SAPP - ts % SAPP : 0;     // start offset

    if(is + os > Sample::csi){                  // if not enough samples
        page = pages = 0;                       // 
        drawStatus();                           // print status only
        return;                                 //
    }

    uint16_t oe = te % SAPP;                    // end offset

    is += os;                                   // correct indices and times
    ie -= oe;                                   //
    ts += os;                                   //
    te -= oe;                                   // 

    pages = (Sample::csi - oe) / PAGSI;         // complete pages
    page = pages ? is / PAGSI + 1 : 0;          // page
    if(page > pages) page = pages;              //

    drawStatus();                               // draw
    drawTimeStamps();                           //
    drawSamples();                              //
}

void View::countFails()
{
    static bool fdi=true, fmi=true, fdo=false, fmo=false;       // fail dis/mis in/out
    uint8_t stat = Sample::samples[Sample::csi].status;

    if(stat & ERR_DIS){
        if(fdi) nDis++;
        fdi = false;
    }
    else{
        fdi = true;
    }

    if(stat & ERR_MIS){
        if(fmi) nMis++;
        fmi = false;
    }
    else{
        fmi = true;
    }

    if(Sample::preShift){
        stat = Sample::samples[0].status;

        if(stat & ERR_DIS){
            fdo = true;        
        }    
        else{
            if(fdo) nDis--;
            fdo = false;
        }

        if(stat & ERR_MIS){
            fmo = true;        
        }    
        else{
            if(fmo) nMis--;
            fmo = false;
        }        
    }
}

void View::drawStatus()
{
	DateTimeDcf now = Dcf::now();

    sprintf(buf, "%us %s", views[cView].sapp, views[cView].td);                 // interval + time
	Text::drawText(buf, 0, 0, mains_10_7, TOPLEFT);                         

    sprintf(buf, "%c%u/%u", vsi<0 ? ' ' : '*'  ,page, pages);                   // * / page / pages
	Text::drawText(buf, 8*FBW, 0, mains_10_7, TOPLEFT);

    countFails();                                                               // count disturbances and missing waves

    if(nDis){                                                                   // number of disturbances
        sprintf(buf, "%c%s%u", CDIS, nDis<100 ? "" : ">", nDis<100 ? nDis : 99);
	    Text::drawText(buf, 0, FBH+2, mains_8_5, TOPLEFT);
    }

    if(nMis){                                                                   // number of missing waves
        sprintf(buf, "%c%s%u", CMIS, nMis<100 ? "" : ">", nMis<100 ? nMis : 99);
	    Text::drawText(buf, 35, FBH+2, mains_8_5, TOPLEFT);
    }

	sprintf(buf, "%02u:%02u:%02u", now.hour(), now.minute(), now.second());     // dcf time
	Text::drawText(buf, DWIDTH-67, 0, mains_10_7, TOPLEFT);

	uint32_t m = (Dcf::secs - Dcf::last) / 60;                                  // last sync time

    if(m > 70){
        uint32_t h = m / 60;

        if(h < 100)
            sprintf(buf, "%uh", h);
        else
            sprintf(buf, ">99h");

        Text::drawText(buf, DWIDTH - Text::getTextWidth(buf, mains_8_5), FBH+2, mains_8_5, TOPLEFT);
    }

	// uint32_t m = (Dcf::secs - Dcf::fist) / 60;                                  // on time
	// sprintf(buf, "%ud%uh%um", (uint16_t)(m/1440), (uint16_t)(m/60)%24, (uint16_t)(m%60));
	// Text::drawText(buf, DWIDTH-67, FBH+2, mains_8_5, TOPLEFT);
}

void View::drawTimeStamps()
{
    uint16_t nxp = 0;                                       // next possible x pos           
    lwd = 255;                                              // reset last weekday

    for(uint32_t t=ts+SAPP, x=0; t<te+SAPP; t+=SAPP, x++){  // samples in display
        if(x >= nxp){                                       // stamp possible?
            if(t % MIMO == 0){                              // full minute modulo match
                timeStamp(x, t);                            // print stamp              12345
                nxp = x + 11 * FBW / 2;                     // next possible stamp pos, hh:mm, 11/2 = 5.5
            }
        }
    }           
}

void View::timeStamp(uint16_t x, uint32_t t)
{
    if(x>0 && x<VW-1)                                   // line pos range
        Draw::line(x, FBH*2+1, x, DHEIGHT-1, PAT21);
    
    if(x>0 && x<VW-3){                                  // text pos range
        DateTimeDcf dtd = DateTimeDcf(t);               // construct class
        uint8_t wd = dtd.dayOfWeek();                   // calc weekday
        
        if(wd != lwd){                                  // new weekday?
            sprintf(buf, "%s", dtd.dayOfWeekStr(wd));
            Text::drawText(buf, x+3, DHEIGHT-FBH*2-2, mains_8_5, TOPLEFT); 
            lwd = wd;
        }

        sprintf(buf, "%u:%02u", dtd.hour(), dtd.minute());
        Text::drawText(buf, x+3, DHEIGHT-FBH-2, mains_8_5, TOPLEFT); 
    }
}

void View::drawSamples()
{    
    uint16_t y, y0;

    for(uint32_t i=is+1, x=0; i<ie+1; i+=SAPP, x++){
        y = GRAZE + getSample(i);

        if(!status){
            if(x == 0)
                Draw::setPixel(x, y, COLBLACK);
            else
                Draw::line(x-1, y0, x, y, COLBLACK);
        }
        else{
            if(status & ERR_MIS)
                Draw::line(x, 2*FBH+1, x, DHEIGHT-2*FBH-1, PATMIS);
            else if(status & ERR_DIS)
                Draw::line(x, 2*FBH+1, x, DHEIGHT-2*FBH-1, PATDIS);
        }

        y0 = y;
    }
}

int8_t View::getSample(uint16_t si)
{
    int32_t sum = 0;
    status = 0;

    for(uint32_t i=si; i<si+SAPP; i++){
        sum += Sample::samples[i].sample;
        status |= Sample::samples[i].status;
    }

    return sum / SAPP;
}

void View::prevPage()
{
    if(vsi == -1)
        vsi = Sample::csi - PAGSI - PAGAD;
    else
        vsi -= PAGAD;

    if(vsi < 0) 
        vsi = 0;
}

void View::nextPage()
{
    if(vsi != -1){
        vsi += PAGAD;
        
        if(vsi > Sample::csi - PAGSI) 
            vsi = Sample::csi - PAGSI;
    }
}

void View::firstPage()
{
    vsi = 0;
}

void View::lastPage()
{
    vsi = -1;
}

void View::prevView()
{
    if(--cView < 0) 
        cView = NUMVIEWS - 1;  
}

void View::nextView()
{
    if(++cView >= NUMVIEWS) 
        cView = 0;
}

// -----------------------------------------------------------------------------

// void View::countFails()
// {
//     static bool f1=true, f2=false;
//     uint8_t stat = Sample::samples[Sample::csi].status;
// 
//     if(stat){
//         if(f1){
//             if(stat & ERR_DIS) nDis++;
//             if(stat & ERR_MIS) nMis++;
//         }
// 
//         f1 = false;
//     }
//     else{
//         f1 = true;
//     }
// 
//     if(Sample::preShift){
//         stat = Sample::samples[0].status;
// 
//         if(stat){
//             f2 = true;
//         }
//         else{
//             if(f2){
//                 if(stat & ERR_DIS) nDis--;
//                 if(stat & ERR_MIS) nMis--;
//             }
// 
//             f2 = false;
//         }
//     }
// }