\ module for reading the real time clock
hex

scope{
auxiliary
( define constants )
: IO_RTCLOCK_DATE    FFFFFEFC ; inl
: IO_RTCLOCK_TIME    FFFFFEF8 ; inl
: IO_RTCLOCK_OTHER   FFFFFEF4 ; inl

public
\ Get the current date
: rtc-date ( -- n )  IO_RTCLOCK_DATE @ ; inl

\ Get the current time
: rtc-time ( -- n )  IO_RTCLOCK_TIME @ ; inl

\ Get other information
: rtc-other ( -- n ) IO_RTCLOCK_OTHER @ ; inl
}scope
