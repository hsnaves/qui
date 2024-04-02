\ main internal words
hex

\ compile a value in the code buffer
: , ( v -- )  here %, tail ; noexit

\ compile a byte in the code buffer
: c, ( b -- ) here %c, tail ; noexit

\ compiles a string in the code buffer
: str, ( c-str n -- )  here %str, tail ; noexit

\ computes the relative address for jumps
: reladdr ( target n -- v n )
  >r here @
  r@ 1+ + - r>
  ;

scope{
private
\ checks if a given value fits a literal of fixed size
: litn? ( v n -- b )
  7 * 1-
  over swap
  signe =
  ;

\ checks if the jump fits the given size of literal
: litjn? ( target n -- b )
  reladdr
  litn? tail
  ; noexit

public
\ obtains the size of a literal
: litn ( v -- v n )
  1
  begin
    2dup litn? =0
    if 1+ again then
  end
  ;

\ obtains the size of a literal for jump
: litjn ( target -- target n )
  1
  begin
    2dup litjn? =0
    if 1+ again then
  end
  ;

private
\ shows an error for large literal string
," large literal"
: literror ( -- )
  [ swap ] lit lit
  error tail
  ; noexit

public
\ compile a literal of a given fixed size n
: litn, ( v n -- )
  2dup litn? =0
  if literror tail then
  begin
    1- dup =0
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

\ compiles a literal for a jump of a given fixed size
: litjn, ( target n -- )
  2dup litjn? =0
  if literror tail then
  reladdr
  litn, tail
  ; noexit
}scope

\ compile a literal
: lit, ( v -- )
  litn litn, tail
  ; noexit

\ compiles a jump of a given fixed size
: jumpn, ( target n insn -- )
  >r litjn, r> c, tail
  ; noexit

\ compiles a jump to a target
: jump, ( target insn -- )
  >r litjn r> jumpn, tail
  ; noexit

\ compiles a literal for jump of automatic size
: litja, ( target -- )
  janum c@ litjn, tail
  ; noexit

( words defining some useful instructions )
: I_RET    C0 ; inl
: I_JSR    C1 ; inl
: I_JMP    C2 ; inl
: I_JZ     C3 ; inl

( define constants for flags )
: F_EXT    80 ; inl
: F_IMM    40 ; inl
: F_INL    20 ; inl
: F_LINK   10 ; inl
: F_XT     08 ; inl

\ compile a sequence of instructions inline
: inline, ( addr -- )
  dup I_RET char-find
  str, tail
  ; noexit

\ compiles a return
: exit, ( -- )
  I_RET c, tail
  ; noexit
