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
   line  \ flush the current line
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
   dup
   if onexcept @ exec tail then
   drop
   0 rsp ! 1 dsp !
   onboot @ exec \ assumes onboot is non-zero
   quit tail
   ; noexit


\ write the initial jump to boot
here @
0 here !
' boot 3 I_JMP jumpn,
here !

