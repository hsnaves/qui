\ implementation of the boot, quit and related words

hex

\ goes back to the interpreter
: quit ( -- )
   0 rsp !
   0 channel c!
   interpreter tail
   ; noexit

( *** boot *** )

scope{
private

\ default implementation of error
: default_error ( c-str n -- )
   1 channel c!
   type nl
   line                         \ flush the current line
   quit tail
   ; noexit
last @ >xt is error

\ default implementation of onexcept
: default_onexcept ( status -- )
   terminate tail
   ; noexit
last @ >xt onexcept !

}scope

: boot ( status -- )
   dup                          \ d: status status
   if onexcept @ exec tail then
   drop                         \ d:
   0 rsp ! 1 dsp !              \ reset the data and return stack
   onboot @ exec                \ assumes onboot is non-zero
   quit tail
   ; noexit


here @                          \ d: vhere
0 here !                        \ d: vhere
' boot 3 I_JMP jumpn,           \ write the jump
here !                          \ restore old here

