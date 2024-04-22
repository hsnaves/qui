#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "vm/quivm.h"
#include "dev/rtclock.h"

/* Functions */

int rtclock_init(struct rtclock *rtc)
{
    rtc->use_utc = 0;
    return 0;
}

void rtclock_destroy(struct rtclock *rtc)
{
    (void)(rtc); /* UNUSED */
}

void rtclock_configure(struct rtclock *rtc, int use_utc)
{
    rtc->use_utc = use_utc;
}

uint32_t rtclock_read_callback(struct rtclock *rtc,
                               struct quivm *qvm, uint32_t address)
{
    time_t secs;
    struct tm *t, zt = { 0 };
    uint32_t v;

    (void)(qvm); /* UNUSED */

    secs = time(NULL);
    t = (rtc->use_utc) ? gmtime(&secs) : localtime(&secs);
    if (!t) t = &zt;

    switch (address) {
    case IO_RTCLOCK_DATE:
        v = (t->tm_year + 1900) * 10000;
        v += (t->tm_mon + 1) * 100;
        v += t->tm_mday;
        break;
    case IO_RTCLOCK_TIME:
        v = t->tm_hour * 10000;
        v += t->tm_min * 100;
        v += t->tm_sec;
        break;
    case IO_RTCLOCK_OTHER:
        v = t->tm_yday * 10000;
        v += t->tm_wday * 100;
        v += (t->tm_isdst) ? 1 : 0;
        break;
    default:
        v = -1;
        break;
    }

    return v;
}

void rtclock_write_callback(struct rtclock *rtc, struct quivm *qvm,
                            uint32_t address, uint32_t v)
{
    (void)(rtc); /* UNUSED */
    (void)(qvm); /* UNUSED */
    (void)(address); /* UNUSED */
    (void)(v); /* UNUSED */
}
