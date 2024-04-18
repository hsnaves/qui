\ interpret word code
hex

scope{
private

\ word to use when interpreting
: (interpret) ( c-str n -- ? )
  0 lookup
  dup if >xt >r 2drop exit then
  drop \ let it fall through to (number)
  ; noexit

\ parses a number with error validation
: (number) ( c-str n -- num )
  2dup number
  if drop unknown tail then
  nip nip
  ;

\ word to use when compiling
: (compile) ( c-str n -- )
  1 lookup
  dup if dup c@ F_IMM and =0 and then
  dup =0 if
    lookup dup =0
    if drop (number) lit, tail then
  then
  >r 2drop
  r@ >xt r> >flags
  dup F_IMM and
  if drop >r exit then
  F_INL and if inline, tail then
  I_JSR jump, tail
  ; noexit

public
\ interprets a single word ( given as counted string )
: interpret ( c-str n -- )
  state c@ if (compile) tail then
  (interpret) tail
  ; noexit
}scope

