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

\ compile a literal
: lit, ( v -- )
   begin
      dup dup                   \ d: v v v
      6 sge =                   \ d: v se6?
      if
         3F and 80 +            \ d: v'
         c, exit                \ d:
      then
      dup 7 shr                 \ d: v {v>>7}
      recurse                   \ d: v
      7F and c, tail            \ d:
   end
   ; noexit

\ compile a literal of a given fixed size n
: litn, ( v n -- )
   begin
      1- dup                    \ d: v n' n'
      =0 if
         drop                   \ d: v
         3F and 80 +            \ d: v'
         c, tail
      then
      over 7 shr                \ d: v n {v>>7}
      swap recurse              \ d: v
      7F and c, tail            \ d:
   end
   ; noexit

\ checks if a given value fits a literal of fixed size
: litn? ( v n -- b )
   7 * 1-                       \ d: v bits
   over swap                    \ d: v v bits
   sge =                        \ d: b
   ;

\ compiles a jump to a target
: jump, ( target insn -- )
   >r                           \ d: target | r: insn
   here @                       \ d: target vhere | r: insn
   1+ -                         \ d: diff | r: insn
   1                            \ d: diff n | r: insn
   begin
      2dup -                    \ d: diff n v | r: insn
      over litn?                \ d: diff n ok? | r: insn
      =0 if
         1+ again
      then
   end
   swap over -                  \ d: n diff' | r: insn
   swap litn,                   \ d: | r: insn
   r> c, tail                   \ d:
   ; noexit

\ compiles a jump of a given fixed size
: jumpn, ( target n insn -- )
   >r >r                        \ d: target | r: insn n
   here @                       \ d: target vhere | r: insn n
   r@ 1+ + -                    \ d: v | r: insn n
   r> litn,                     \ d: | r: insn
   r> c, tail                   \ d:
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
