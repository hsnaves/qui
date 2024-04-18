\ meta interpreter
hex

scope{
private
\ parses a number with error validation
: (number) ( c-str n -- num )
  2dup number
  if drop unknown tail then
  nip nip
  ;

\ word to use when interpreting
: (interpret) ( c-str n -- )
  swap addr>host swap
  0 h:lookup
  dup if h:>xt >r 2drop exit then
  drop (number) tail
  ; noexit

\ word to compile using host dictionaries
: (compile-host) ( c-str n -- )
  swap addr>host swap
  0 h:lookup
  dup if
    dup h:>flags h:F_IMM and
    if h:>xt >r 2drop exit then
  then
  drop (number) lit, tail
  ; noexit

\ word to use when compiling
: (compile) ( c-str n -- )
  1 lookup
  dup if dup c@ F_IMM and =0 and then
  dup =0 if
    lookup dup =0
    if drop (compile-host) tail then
  then
  dup >flags
  dup F_IMM and
  if 2drop (compile-host) tail then
  rot drop rot drop
  swap >xt swap F_INL and
  if inline, tail then
  I_JSR jump, tail
  ; noexit

\ interprets a single word ( given as counted string )
: meta-interpret ( c-str n -- )
  state c@ if (compile) tail then
  (interpret) tail
  ;

public
\ meta Forth interpreter
\ it interprets all the words in an infinite loop
\ except when the :state turns to zero
: meta-interpreter ( -- )
  0 state c!
  begin
    word meta-interpret
    again
  end
  ;
}scope
