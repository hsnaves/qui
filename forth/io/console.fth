\ implementation of console words

hex

scope{
auxiliary

( *** constants for the console *** )
: IO_CONSOLE_IN      FFFFFFBC ; inl
: IO_CONSOLE_OUT     FFFFFFB8 ; inl
: IO_CONSOLE_ERR     FFFFFFB4 ; inl

public

\ emits one character to the standard output
: (emit) ( c -- )
   IO_CONSOLE_OUT
   channel c@                   \ select based on the channel
   if 4 - then
   !
   ;
' (emit) is emit

\ obtains an input character from standard input
: (getc) ( -- c )
   IO_CONSOLE_IN @
   ;
' (getc) is getc

}scope


