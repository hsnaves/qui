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

\ performs the boot
: boot ( status -- )
  dup if onexcept @ >r exit then
  drop
  0 rsp ! 1 dsp !
  onboot @ exec \ assumes onboot is non-zero
  quit tail
  ; noexit

\ write the initial jump to boot
jsize c@ here @ 0 here ! 3 jsize c!
' boot litj, I_JMP c,
here !  \ restore here
jsize c! \ restore jsize

private
\ default implementation of prompt
: default_prompt ( -- )
  ;
last @ >xt is prompt

\ default implementation of error
: default_error ( c-str n severity -- )
  >r 1 channel c! type
  r> 1 over u< if nl 1 - status! tail then
  if nl line quit tail then
  ;
last @ >xt is error

\ default implementation of onexcept
: default_onexcept ( status -- )
  status! tail
  ; noexit
last @ >xt onexcept !
}scope
