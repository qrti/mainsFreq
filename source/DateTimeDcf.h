#pragma once

#define SECONDS_PER_DAY         86400L

#include <stdlib.h>
#include "pico/stdlib.h"

class DateTimeDcf
{
public:
    DateTimeDcf(uint32_t t=0);
    DateTimeDcf(uint16_t year, uint8_t month, uint8_t day, uint8_t hour=0, uint8_t min=0, uint8_t sec=0);
    DateTimeDcf(const char* date, const char* time);

    uint16_t year() const       { return 2000 + yOff; }
    uint8_t month() const       { return m; }
    uint8_t day() const         { return d; }
    uint8_t hour() const        { return hh; }
    uint8_t minute() const      { return mm; }
    uint8_t second() const      { return ss; }
    uint8_t dayOfWeek() const;
	const char* dayOfWeekStr(uint8_t) const;

    uint32_t get() const;		// 32-bit time as seconds since 01.01.2000

protected:
    uint8_t yOff, m, d, hh, mm, ss;
};
