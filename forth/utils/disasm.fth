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

\ disassemble a literal shift instruction
," LITS "
: disasm_lits ( opc -- )
   [ swap ] lit lit
   type                         \ d: opc
   b. tail
   ; noexit

\ disassemble a literal instruction
," LIT  "
: disasm_lit ( opc -- )
   [ swap ] lit lit             \ get the string
   type                         \ d: opc
   80 -                         \ d: num
   6 signe                      \ d: num'
   w. tail
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
: disasm1 ( addr -- )
   dup w. space                \ d: addr
   c@ dup b.                   \ d: insn
   5 spaces                    \ d: insn
   disasm_insn                 \ d:
   nl tail                     \ d:
   ; noexit

public

\ disassembles many instructions at a given address
\ returns the address after the last decoded instruction
: disasm ( addr n -- addr' )
   begin
      dup if
         1- swap               \ d: n' addr
         dup disasm1           \ d: n addr
         1+ swap               \ d: addr' n
         again
      then
   end
   drop                        \ d: addr
   ;

}scope
