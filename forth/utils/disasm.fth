\ module for disassembling the QUI opcodes

hex

scope{
public

( the opcode table )
here @
," RET  JSR  JMP  JZ   EQ0  EQ   ULT  LT   " 2drop
," NOP  AND  OR   XOR  ADD  SUB  UMUL UDIV " 2drop
," RD   WRT  RDB  WRTB SIGNESHL  USHR SHR  " 2drop
," DUP  DROP SWAP OVER ROT  RTO  RFROMRPEEK" 2drop
," INVL " 2drop

auxiliary

\ opcodes points to the table defined above
: opcodes ( -- addr )
   lit
   ; inl

private

\ checks if the address contains a literal
\ returns the address of the first opcode and a boolean
\ indicating that it is a literal
: literal? ( addr -- addr' b )
   begin
      dup c@                    \ d: addr opc
      80 u<                     \ d: addr lits?
      if 1- again then
   end
   dup c@ 80 BF within tail     \ check for lit
   ; noexit

\ checks if this is the last literal in a sequence
: lastliteral? ( addr -- addr' b )
   1+                           \ d: addr'
   dup c@                       \ d: addr opc
   80 u<                        \ d: addr lits?
   if 0 exit then               \ return addr 0
   1- literal? tail
   ; noexit

\ checks if this is a jump with known address
: jumpliteral? ( addr -- addr b )
   dup c@                       \ d: addr opc
   I_JSR I_JZ within            \ d: addr jump?
   =0 if 0 exit then            \ return addr 0
   1- literal? tail
   ; noexit

\ opbtains the literal value from an opcode
: litval ( opc -- n )
   80 - 6 signe
   ;

\ obtains the value of the literal starting at address
: literal ( start end -- n )
   >r                           \ d: start | r: end
   dup c@ litval                \ d: start n | r: end
   begin
      swap 1+                   \ d: n pos' | r: end
      tuck                      \ d: pos n pos | r: end
      r@ u<=                    \ d: pos n remain? | r: end
      if
         7 shl                  \ d: pos n' | r: end
         over c@ or             \ d: pos n' | r: end
         again
      then
   end
   swap r> 2drop                \ d: n
   ;

: print-literal ( addr -- )
   dup lastliteral?             \ d: end start lit?
   =0 if 2drop exit then
   swap literal                 \ d: num
   [ char ( ] lit emit          \ d: num
   . [ char ) ] lit emit tail   \ d:
   ; noexit

: print-jump ( addr -- )
   dup                          \ d: addr endp1
   dup jumpliteral?             \ d: addr endp1 start lit?
   =0 if 2drop drop exit then
   swap 1- literal              \ d: addr num
   + 1+                         \ d: target
   w. tail                      \ d:
   ; noexit

\ disassemble a literal shift instruction
," LITS "
: disasm_lits ( addr opc -- )
   [ swap ] lit lit
   type                         \ d: addr opc
   b.                           \ d: addr
   space
   print-literal tail
   ; noexit

\ disassemble a literal instruction
," LIT  "
: disasm_lit ( addr opc -- )
   [ swap ] lit lit             \ get the string
   type                         \ d: opc
   litval                       \ d: addr n
   b.                           \ d: addr
   space
   print-literal tail
   ; noexit

\ disassemble a regular instruction
: disasm_reg ( addr opc -- )
   C0 -                         \ d: addr opc'
   20 over u<                   \ d: addr opc large?
   if drop 20 then              \ d: addr opc'
   5 * opcodes + 5              \ d: addr c-str n
   type                         \ d: addr
   print-jump tail
   ; noexit

\ disassemble a single instruction
: disasm_insn ( addr opc -- )
   dup 80 u<                    \ d: addr opc litshift?
   if disasm_lits tail then     \ d: addr opc
   dup C0 u<                    \ d: addr opc lit?
   if disasm_lit tail then
   disasm_reg tail
   ;

\ fully disassemble a single instruction at given address
: disasm1 ( addr -- )
   dup                         \ d: addr addr
   [ wordbuf buf>off ] lit     \ d: addr addr off
   @ -                         \ d: addr addr'
   w. 2 spaces                 \ d: addr
   dup c@ dup b.               \ d: addr opc
   5 spaces                    \ d: addr opc
   disasm_insn                 \ d:
   nl tail                     \ d:
   ; noexit

public

\ disassembles many instructions at a given address
\ returns the address after the last decoded instruction
: disasm ( addr n -- addr' )
   begin
      dup if
         over disasm1
         1 /str                \ d: addr' n'
         again
      then
   end
   drop                        \ d: addr
   ;

}scope
