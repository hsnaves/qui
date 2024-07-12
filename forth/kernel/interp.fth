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
  dup c@ F_IMM and =0 and
  dup =0 if
    lookup dup =0
    if drop number lit, tail then
  then
  >r drop drop r>
  dup >xt swap >flags
  dup F_IMM and
  if drop >r exit then
  F_INL and if inline, tail then
  I_JSR jump, tail
  ; noexit

public
internal current !
\ interprets a single word ( given as counted string )
align defer interpret ( c-str n -- )
forth current !

private
\ default implementation of interpret
: (interpret) ( c-str n -- )
  state c@ if (do-compile) tail then
  (do-interpret) tail
  ; noexit
last @ >xt is interpret

public
\ main Forth interpreter
\ it interprets all the words in an infinite loop
: interpreter ( -- )
  0 state c!
  begin
    word interpret
    again
  end
  ;

}scope

