\ main internal words
hex

scope{
private
\ computes the relative address for jumps
: reladdr ( target n -- v n )
  swap over here @ + 1 + - swap
  ;

\ checks if a given value fits a literal of fixed size
\ note: the literal might be a target address
: fit? ( v n a? -- b )
  if reladdr then
  7 * 1 -
  over swap
  20 swap - dup >r shl r> shr
  = =0
  ;

\ obtains the size of a literal
: get-size ( v a? -- v n )
  >r 1
  begin
    2dup 0 r@ fit?
    if 1 + again then
  end
  r> drop
  ;

\ shows an error for large literal string
" large literal"
: print-large-error ( -- )
  [ swap ] lit lit
  1 error tail
  ; noexit

\ compile a literal of a given fixed size
: compile-literal ( v n a?-- )
  if reladdr then
  2dup 0 fit?
  if print-large-error tail then
  begin
    1 - dup =0
    if
      drop
      3F and 80 +
      c, tail
    then
    over 7 shr
    swap recurse
    7F and c, tail
  end
  ; noexit

public
\ compile a literal
: l, ( v -- )
  0 get-size 0 compile-literal tail
  ; noexit

\ compiles a literal for jump of automatic size
\ based on jsize
: lj, ( target -- )
  jsize c@ 1 compile-literal tail
  ; noexit

\ compiles a jump to a target
: j, ( target insn -- )
  >r 1 get-size 1 compile-literal r> c, tail
  ; noexit

ephemeral
include" forth/bootstrap/flags.fth"

public
( define constants for control instructions )
: RET    C0 ; inl
: JSR    C1 ; inl
: JMP    C2 ; inl
: JZ     C3 ; inl

( public flags )
: IMM    IMM ; inl
: INL    INL ; inl

}scope

