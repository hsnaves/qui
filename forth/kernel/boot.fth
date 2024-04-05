\ implementation of the boot, quit and related words
hex

( *** boot *** )
scope{
public
\ goes back to the interpreter
: quit ( -- )
  0 -1 rstack !
  0 channel c!
  interpreter tail
  ; noexit

: boot ( status -- )
  dup
  if onexcept @ exec tail then
  drop
  0 -1 rstack ! 1 -1 dstack !
  onboot @ exec \ assumes onboot is non-zero
  quit tail
  ; noexit

\ write the initial jump to boot
here @ 0 here !
' boot 3 I_JMP jumpn,
here !  \ restore here

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
