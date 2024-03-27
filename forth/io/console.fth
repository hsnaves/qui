\ implementation of console words

hex

scope{
auxiliary

( *** constants for the console *** )
: IO_CONSOLE_IN      FFFFFFBC ; inl
: IO_CONSOLE_OUT     FFFFFFB8 ; inl
: IO_CONSOLE_CHANNEL FFFFFFB4 ; inl

public

\ selects the channel for output
: channel ( -- addr ) IO_CONSOLE_CHANNEL ; inl

\ emits one character to the standard output
: (emit) ( c -- )
   IO_CONSOLE_OUT !
   ;
' (emit) is emit

\ obtains an input character from standard input
: (getc) ( -- c )
   IO_CONSOLE_IN @
   ;
' (getc) is getc

}scope


