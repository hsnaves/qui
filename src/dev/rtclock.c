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
    case IO_RTCLOCK_YEAR:
        v = t->tm_year + 1900;
        break;
    case IO_RTCLOCK_MONTH:
        v = t->tm_mon + 1;
        break;
    case IO_RTCLOCK_MDAY:
        v = t->tm_mday;
        break;
    case IO_RTCLOCK_HOUR:
        v = t->tm_hour;
        break;
    case IO_RTCLOCK_MIN:
        v = t->tm_min;
        break;
    case IO_RTCLOCK_SEC:
        v = t->tm_sec;
        break;
    case IO_RTCLOCK_WDAY:
        v = t->tm_wday;
        break;
    case IO_RTCLOCK_YDAY:
        v = t->tm_yday;
        break;
    case IO_RTCLOCK_ISDST:
        v = t->tm_isdst ? -1 : 0;
        break;
    default:
        v = -1;
        break;
    }

    return v;
}

void rtclock_write_callback(struct rtclock *rtc,  struct quivm *qvm,
                            uint32_t address, uint32_t v)
{
    (void)(rtc); /* UNUSED */
    (void)(qvm); /* UNUSED */
    (void)(address); /* UNUSED */
    (void)(v); /* UNUSED */
}

