\ module for reading the real time clock

hex

scope{
auxiliary

( define constants )
: IO_RTCLOCK_YEAR   FFFFFEFC ; inl
: IO_RTCLOCK_MONTH  FFFFFEF8 ; inl
: IO_RTCLOCK_MDAY   FFFFFEF4 ; inl
: IO_RTCLOCK_HOUR   FFFFFEF0 ; inl
: IO_RTCLOCK_MIN    FFFFFEEC ; inl
: IO_RTCLOCK_SEC    FFFFFEE8 ; inl
: IO_RTCLOCK_WDAY   FFFFFEE4 ; inl
: IO_RTCLOCK_YDAY   FFFFFEE0 ; inl
: IO_RTCLOCK_ISDST  FFFFFEDC ; inl

private

," SunMonTueWedThuFriSat"

\ string with the weekdays
: weekdays [ drop lit, ] ; inl

: print-weekday ( n -- )
   3 * weekdays + 3 type tail
   ; noexit

\ prints a number with 2 digits
: print-2dig ( n -- )
   base c@ u/mod u. u.
   ;

\ prints the dash character
: print-dash ( -- )
   [ char - lit, ] emit tail
   ; noexit

\ prints the colon character
: print-colon ( -- )
   [ char : lit, ] emit tail
   ; noexit

,"  DST"
\ print the DST string
: print-dst ( -- )
   [ swap lit, lit, ] type tail
   ; noexit

public

\ Get the current year
: rtc-year ( -- n )
   IO_RTCLOCK_YEAR @
   ; inl

\ Get the current month (1-12)
: rtc-month ( -- n )
   IO_RTCLOCK_MONTH @
   ; inl

\ Get the current day (1-31)
: rtc-day ( -- n )
   IO_RTCLOCK_MDAY @
   ; inl

\ Get the current hour (0-23)
: rtc-hour ( -- n )
   IO_RTCLOCK_HOUR @
   ; inl

\ Get the current minute (0-23)
: rtc-minute ( -- n )
   IO_RTCLOCK_MIN @
   ; inl

\ Get the current second (0-59)
: rtc-second ( -- n )
   IO_RTCLOCK_SEC @
   ; inl

\ Get the weekday (0-6)
: rtc-weekday ( -- n )
   IO_RTCLOCK_WDAY @
   ; inl

\ Get the weekday (0-365)
: rtc-yearday ( -- n )
   IO_RTCLOCK_YDAY @
   ; inl

\ Get the daylight savings indicator
: rtc-isdst ( -- n )
   IO_RTCLOCK_ISDST @
   ; inl

\ prints the date and time (assumes decimal base)
: rtc-print ( -- )
   rtc-weekday print-weekday space
   rtc-year u. print-dash
   rtc-month print-2dig print-dash
   rtc-day print-2dig space
   rtc-hour print-2dig print-colon
   rtc-minute print-2dig print-colon
   rtc-second print-2dig
   rtc-isdst if print-dst then
   ;

}scope

decimal
