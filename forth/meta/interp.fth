\ meta interpreter
hex

2 janum c! \ to compile large if

\ interprets a single word ( given as counted string )
: interpret ( c-str n -- )
  2dup state c@ lookup          \ d: c-str n addr
  dup if
    dup >xt
    swap >flags
    dup F_IMM and
    state c@ =0 or =0           \ d: c-str n xt fl imm?
    if
      F_INL and
      if inline, else I_JSR jump, then
      2drop exit
    then
    drop
  then
  drop                          \ d: c-str n

  2dup
  swap addr>host swap
  state c@ h:lookup
  dup if
    dup h:>xt
    swap h:>flags
    h:F_IMM and
    state c@ =0 or
    if
      nip nip
      exec tail
    then
  then
  drop

  2dup number                   \ d: c-str n num rem
  if drop unknown tail then
  nip nip
  state c@
  if lit, then
  ;

1 janum c!

\ meta Forth interpreter
\ it interprets all the words in an infinite loop
\ except when the :state turns to zero
: interpreter ( -- )
  0 state c!
  begin
    word interpret
    again
  end
  ;
