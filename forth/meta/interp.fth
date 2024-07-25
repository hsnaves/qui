\ meta interpreter
hex

scope{
private
\ word to use when interpreting
: (interpret) ( c-str n -- )
  swap addr>host swap
  0 h:lookup
  dup if h:>xt >r drop drop exit then
  drop number tail
  ; noexit

\ word to compile using host dictionaries
: (compile-host) ( c-str n -- )
  swap addr>host swap
  0 h:lookup
  dup if
    dup h:>flags h:IMM and
    if h:>xt >r drop drop exit then
  then
  drop number l, tail
  ; noexit

\ word to use when compiling
: (compile) ( c-str n -- )
  1 lookup
  dup if dup c@ IMM and =0 and then
  dup =0 if
    lookup dup =0
    if drop (compile-host) tail then
  then
  dup >flags
  dup IMM and
  if drop drop (compile-host) tail then
  rot drop rot drop
  swap >xt swap INL and
  if dup RET index s, tail then
  JSR j, tail
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
