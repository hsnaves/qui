\ interpret word code
hex

scope{
private
\ word to use when interpreting
: (do-interpret) ( c-str n -- ? )
  0 lookup
  dup if >xt >r drop drop exit then
  drop number tail
  ; noexit

\ word to use when compiling
: (do-compile) ( c-str n -- )
  1 lookup
  dup c@ IMM and =0 and
  dup =0 if
    lookup dup =0
    if drop number l, tail then
  then
  >r drop drop r>
  dup >xt swap >flags
  dup IMM and
  if drop >r exit then
  INL and if dup RET index s, tail then
  JSR j, tail
  ; noexit

public
\ default implementation of interpret
: interpret ( c-str n -- )
  state c@ if (do-compile) tail then
  (do-interpret) tail
  ; noexit

}scope

