\ basic compiler words

hex

\ compile a value in the code buffer
: , ( v -- )
   here                         \ d: v here
   %, tail
   ; noexit

\ compile a byte in the code buffer
: c, ( b -- )
   here                         \ d: b here
   %c, tail
   ; noexit

\ compiles a string in the code buffer
: str, ( c-str n -- )
   here                         \ d: c-str n here
   %str, tail
   ; noexit

\ computes the relative address for jumps
: reladdr ( target n -- v n )
   >r                           \ d: target | r: n
   here @                       \ d: target vhere | r: n
   r@ 1+ + -                    \ d: v | r: n
   r>                           \ d: v n
   ;

\ checks if a given value fits a literal of fixed size
: litn? ( v n -- b )
   7 * 1-                       \ d: v bits
   over swap                    \ d: v v bits
   signe =                      \ d: b
   ;

\ checks if the jump fits the given size of literal
: litjn? ( target n -- b )
   reladdr                      \ d: v n
   litn? tail                   \ d: b
   ; noexit

\ obtains the size of a literal
: litn ( v -- v n )
   1                            \ d: v n
   begin
      2dup litn? =0             \ d: v n notok?
      if 1+ again then
   end
   ;

\ obtains the size of a literal for jump
: litjn ( target -- target n )
   1                            \ d: target n
   begin
      2dup litjn? =0            \ d: target n notok?
      if 1+ again then
   end
   ;

scope{
private
," large literal"               \ d: c-str n
: literror ( -- )
   [ swap ] lit lit             \ compile the string
   error tail
   ; noexit

public

\ compile a literal of a given fixed size n
: litn, ( v n -- )
   2dup litn? =0                \ d: v n notokay?
   if literror tail then
   begin
      1- dup =0                 \ d: v n' zero?
      if
         drop                   \ d: v
         3F and 80 +            \ d: v'
         c, tail
      then
      over 7 shr                \ d: v n {v>>7}
      swap recurse              \ d: v
      7F and c, tail            \ d:
   end
   ; noexit

\ compiles a literal for a jump of a given fixed size
: litjn, ( target n -- )
   2dup litjn? =0               \ d: target n notokay?
   if literror tail then
   reladdr                      \ d: v n
   litn, tail                   \ d:
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
   dup I_RET char-find          \ d: addr n
   str, tail                    \ d:
   ; noexit

\ compiles a return
: exit, ( -- )
   I_RET c, tail                \ compile return
   ; noexit
