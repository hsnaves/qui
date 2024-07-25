\ implementation of console words
hex

scope{
brief
( *** constants for the console *** )
: IO_CONSOLE_IN      FFFFFFDC ; inl
: IO_CONSOLE_OUT     FFFFFFD8 ; inl
: IO_CONSOLE_CHANNEL FFFFFFD4 ; inl

public
\ obtains an input character from standard input
: (getc) ( -- c )    IO_CONSOLE_IN @ ;
' (getc) is getc

\ emits one character to the standard output
: (emit) ( c -- )    IO_CONSOLE_OUT ! ;
' (emit) is emit

\ selects the channel for output
: chn ( -- addr )    IO_CONSOLE_CHANNEL ; inl

}scope
