\ interpreter word code
hex

2 janum c! \ to compile large if
\ interprets a single word ( given as counted string )
: interpret ( c-str n -- )
  2dup state c@ lookup
  dup if
    dup >xt
    swap >flags
    dup F_IMM and               \ d: c-str n xt flags imm?
    state c@ =0 or
    if
      drop nip nip
      exec tail
    then
    F_INL and
    if inline, else I_JSR jump, then
    2drop exit
  then
  drop                          \ d: c-str n
  2dup number
  if drop unknown tail then
  nip nip
  state c@
  if lit, tail then
  ;
1 janum c!

\ main Forth interpreter
\ it interprets all the words in an infinite loop
: interpreter ( -- )
  0 state c!
  begin
    word interpret
    again
  end
  ;
