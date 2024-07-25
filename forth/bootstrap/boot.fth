\ implementation of the boot, quit and related words
hex

( *** boot *** )
scope{
public
\ goes back to the interpreter
: quit ( -- )
  0 rsp !
  0 state c!
  begin
    word interpret
    again
  end
  ; noexit

\ performs the boot
: boot ( status -- )
  dup if ontrap @ >r exit then
  drop
  0 rsp ! 1 dsp !
  onboot @ exec \ assumes onboot is non-zero
  quit tail
  ; noexit

\ write the initial jump to boot
jsz c@ here @ 0 here ! 3 jsz c!
' boot lj, JMP c,
here !  \ restore here
jsz c! \ restore jsz

private
\ default implementation of error
: default_error ( c-str n severity -- )
  >r 1 chn c! type
  r> 1 over u< if nl 1 - sts! tail then
  if nl line 0 chn c! quit tail then
  ;
last @ >xt is error

\ default implementation of ontrap
: default_ontrap ( status -- )
  sts! tail
  ; noexit
last @ >xt ontrap !
}scope
