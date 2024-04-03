\ module for reading the real time clock
hex

extra current !

scope{
ephemeral
( define constants )
: IO_RTCLOCK_DATE    FFFFFF7C ; inl
: IO_RTCLOCK_TIME    FFFFFF78 ; inl
: IO_RTCLOCK_OTHER   FFFFFF74 ; inl

public
\ Get the current date
: rtc-date ( -- n )  IO_RTCLOCK_DATE @ ; inl

\ Get the current time
: rtc-time ( -- n )  IO_RTCLOCK_TIME @ ; inl

\ Get other information
: rtc-other ( -- n ) IO_RTCLOCK_OTHER @ ; inl
}scope

forth current !
