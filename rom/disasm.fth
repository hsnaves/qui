hex

scope{
private

\ dumps a single row of hexadecimal numbers
: dumphex_row ( addr n  -- addr' )
   over w. space                \ d: addr n
   begin
      dup if                    \ d: addr n
        over c@ b. space        \ d: addr n
        str1+                   \ d: addr' n'
        again
      then
   end
   2drop
   ;

\ prints a sequence of 3*n spaces
: dumpspaces_row ( n -- )
   3 * spaces tail
   ; noexit

\ dumps the printable characters
: dumpprint_row ( addr n  -- addr' )
   begin
      dup if                    \ d: addr n
        over c@                 \ d: addr n c
        dup                     \ d: addr n c c
        20 7E within =0         \ d: addr n c notprint?
        if
           drop [ char . lit, ] \ d: addr n c'
        then
        emit
        str1+                   \ d: addr' n'
        again
      then
   end
   2drop
   ;

public

\ dumps the contents of memory at given address
: dump ( addr n -- )
   begin                        \ d: addr n
      dup if
         dup                    \ d: addr n n
         10 over u<             \ d: addr n m large?
         if drop 10 then        \ d: addr n m'
         >r                     \ d: addr n | r: m
         over r@ dumphex_row    \ d: addr n | r: m
         10 r@ - dumpspaces_row \ d: addr n | r: m
         over r@ dumpprint_row  \ d: addr n | r: m
         r@ - swap r> + swap    \ d: addr' n'
         nl again
      then
   end
   2drop
   ;

auxiliary

\ auxiliary function to create the table of opcodes
: define_opcodes ( -- )
   21                           \ d: num
   begin
      word                      \ d: num c-str n
      tuck                      \ d: num n c-str n
      str,                      \ d: num n
      5 swap -                  \ d: num rem
      begin
         dup if                 \ d: num rem
            20 c, 1-            \ d: num rem'
            again
         then
      end
      drop 1-                   \ d: num'
      dup =0 until
   end
   drop
   ;

private

( the opcode table )
here @
define_opcodes
   RET    JSR    JMP    JZ     EQ0    EQ     ULT    LT
   NOP    AND    OR     XOR    ADD    SUB    UMUL   UDIV
   RD     WRT    RDB    WRTB   SGE8   SHL    SHR    SAR
   DUP    DROP   SWAP   OVER   ROT    RTO    RFROM  RPEEK
   INVL

auxiliary

: opcodes ( -- addr )
   [ lit, ]
   ; inl

private

," LITS "
\ disassemble a literal shift instruction
: disasm_lits ( opc -- )
   [ swap lit, lit, ]
   type                         \ d: opc
   b. tail
   ; noexit

," LIT  "
\ disassemble a literal instruction
: disasm_lit ( opc -- )
   [ swap lit, lit, ]
   type                         \ d: opc
   80 - b. tail
   ; noexit

\ disassemble a regular instruction
: disasm_reg ( opc -- )
   C0 -                         \ d: opc'
   20 over u<                   \ d: opc large?
   if drop 20 then              \ d: opc'
   5 * opcodes + 5              \ d: c-str n
   type tail
   ; noexit

\ disassemble a single instruction
: disasm_insn ( opc -- )
   dup 80 u<                    \ d: opc litshift?
   if disasm_lits tail then     \ d: opc
   dup C0 u<                    \ d: opc lit?
   if disasm_lit tail then
   disasm_reg tail
   ;

\ fully disassemble a single instruction at given address
: disasm ( addr -- )
   dup w. space                \ d: addr
   c@ dup b.                   \ d: insn
   5 spaces                    \ d: insn
   disasm_insn                 \ d:
   nl tail                     \ d:
   ; noexit

public

\ disassembles many instructions at a given address
: disassemble ( addr n -- addr' )
   begin
      dup if
         1- swap               \ d: n' addr
         dup disasm            \ d: n addr
         1+ swap               \ d: addr' n
         again
      then
   end
   drop                        \ d: addr
   ;

}scope

decimal
