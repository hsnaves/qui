\ implementation of console words
hex

scope{
ephemeral
( *** constants for the console *** )
: IO_CONSOLE_IN       FFFFFFDC ; inl
: IO_CONSOLE_OUT      FFFFFFD8 ; inl
: IO_CONSOLE_CHANNEL  FFFFFFD4 ; inl

public
\ emits one character to the standard output
: (emit) ( c -- )     IO_CONSOLE_OUT ! ;
' (emit) is emit

\ types a counted string to the standard output
: (type) ( c-str n -- )
  begin
    dup =0
    if 2drop exit then
    over c@ emit
    1/str
    again
  end
  ; noexit
' (type) is type

\ obtains an input character from standard input
: (getc) ( -- c )     IO_CONSOLE_IN @ ;
' (getc) is getc

\ selects the channel for output
: channel ( -- addr ) IO_CONSOLE_CHANNEL ; inl

\ to use the standard error channel for output
: ch_err ( -- ) 1 channel c! ;

\ to use the standard arguments for input
: ch_args  ( -- ) 1 channel 1+ c! ;
}scope
