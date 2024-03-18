\ implementation of the boot, quit and related words

hex

\ goes back to the interpreter
: quit ( -- )
   0 rsp !
   interpreter
   ;

\ implementation of the system words

hex

scope{
auxiliary

( *** constants from the system *** )
: IO_SYS_TERMINATE   FFFFFFE8 ; inl

public

\ terminate the program
: bye ( n -- )
   IO_SYS_TERMINATE !
   ;

}scope

scope{
private

\ default implementation of onerror
: default_onerror ( c-str n -- )
   1 channel c!
   type nl                      \ d:
   0 channel c!
   line                         \ flush the current line
   quit tail
   ; noexit

\ default implementation of onexception
: default_onexception ( status -- )
   bye tail
   ; noexit

' default_onerror is onerror
' default_onexception is onexception

}scope

: boot ( status -- )
   dup                          \ d: status status
   if onexception tail then
   drop                         \ d:
   0 rsp ! 1 dsp !              \ reset the data and return stack
   onboot                       \ assumes onboot is non-zero
   quit tail
   ; noexit


here @                          \ d: vhere
0 here !                        \ d: vhere
' boot 3 I_JMP jumpn,           \ write the jump
here !                          \ restore old here

