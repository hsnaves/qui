\ implementation of the boot, quit and related words
hex

( *** boot *** )
scope{
public
\ goes back to the interpreter
: quit ( -- )
  0 rsp !
  0 channel c!
  interpreter bye tail
  ; noexit

\ leaves the interpreter
: leave ( r: addr1 addr2 addr3 -- r: addr1 )
  rdrop
  ;

\ performs the boot
: boot ( status -- )
  dup if onexcept @ >r exit then
  drop
  0 rsp ! 1 dsp !
  onboot @ exec \ assumes onboot is non-zero
  quit tail
  ; noexit

\ write the initial jump to boot
here @ 0 here !
' boot 3 I_JMP jumpn,
here !  \ restore here

private
\ default implementation of prompt
: default_prompt ( -- )
  ;
last @ >xt is prompt

\ default implementation of error
: default_error ( c-str n -- )
  ch_err type nl
  line  \ flush the current line
  quit tail
  ; noexit
last @ >xt is error

\ default implementation of fatal
: default_fatal ( c-str n -- )
  ch_err type nl
  1 status! tail
  ; noexit
last @ >xt is fatal

\ default implementation of onexcept
: default_onexcept ( status -- )
  status! tail
  ; noexit
last @ >xt onexcept !
}scope
