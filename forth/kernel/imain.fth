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
  signe = =0
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
: lit, ( v -- )
  0 get-size 0 compile-literal tail
  ; noexit

\ compiles a literal for jump of automatic size
\ based on jsize
: litj, ( target -- )
  jsize c@ 1 compile-literal tail
  ; noexit

\ compiles a jump to a target
: jump, ( target insn -- )
  >r 1 get-size 1 compile-literal r> c, tail
  ; noexit

ephemeral
( words defining return instruction )
: I_RET    C0 ; inl

include" forth/kernel/flags.fth"

public
( words defining jump instructions )
: I_JSR    C1 ; inl
: I_JMP    C2 ; inl
: I_JZ     C3 ; inl

( public flags )
: F_IMM    F_IMM ; inl
: F_INL    F_INL ; inl

\ compile a sequence of instructions inline
: inline, ( addr -- )
  dup I_RET char-find
  str, tail
  ; noexit

\ compiles a return
: exit, ( -- )
  I_RET c, tail
  ; noexit
}scope

