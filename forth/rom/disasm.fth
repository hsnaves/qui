\ module for disassembling the QUI opcodes

hex

extra current !

scope{
public
( the opcode table )
" RET  JSR  JMP  JZ   EQ0  EQ   ULT  LT   " drop
" NOP  AND  OR   XOR  ADD  SUB  UMUL UDIV " 2drop
" RD   WRT  RDB  WRTB SIGNESHL  USHR SHR  " 2drop
" DUP  DROP SWAP OVER ROT  RTO  RFROMRPEEK" 2drop
" INVL " 2drop

ephemeral
\ opcodes points to the table defined above
: opcodes ( -- addr ) lit ; inl

private
\ checks if the address contains a literal
\ returns the address of the first opcode and a boolean
\ indicating that it is a literal
: literal? ( addr -- addr' b )
  begin
     dup c@ 80 u<
     if 1- again then
  end
  dup c@ 80 BF within tail      \ check for lit
  ; noexit

\ checks if this is the last literal in a sequence
: lastliteral? ( addr -- addr' b )
  1+ dup c@
  80 u<                         \ d: addr lits?
  if 0 exit then
  1- literal? tail
  ; noexit

\ checks if this is a jump with known address
: jumpliteral? ( addr -- addr b )
  dup c@
  I_JSR I_JZ within =0
  if 0 exit then
  1- literal? tail
  ; noexit

\ opbtains the literal value from an opcode
: litval ( opc -- n )
  80 - 6 signe
  ;

\ obtains the value of the literal starting at address
: literal ( start end -- n )
  >r dup c@ litval
  begin
    swap 1+
    tuck r@ u<=
    if
      7 shl
      over c@ or
      again
    then
  end
  swap r> 2drop
  ;

: print-literal ( addr -- )
  dup lastliteral?
  =0 if 2drop exit then
  swap literal
  [ char ( ] lit emit
  . [ char ) ] lit emit tail
  ; noexit

: print-jump ( addr -- )
  dup dup jumpliteral?
  =0 if 2drop drop exit then
  swap 1- literal + 1+
  w. tail
  ; noexit

\ disassemble a literal shift instruction
" LITS "
: disasm_lits ( addr opc -- )
   [ swap ] lit lit type
   b. space
   print-literal tail
   ; noexit

\ disassemble a literal instruction
" LIT  "
: disasm_lit ( addr opc -- )
  [ swap ] lit lit type
  litval b. space
  print-literal tail
  ; noexit

\ disassemble a regular instruction
: disasm_reg ( addr opc -- )
  C0 - 20 over u<
  if drop 20 then
  5 * opcodes + 5
  type
  print-jump tail
  ; noexit

\ disassemble a single instruction
: disasm_insn ( addr opc -- )
  dup 80 u<
  if disasm_lits tail then
  dup C0 u<
  if disasm_lit tail then
  disasm_reg tail
  ;

\ fully disassemble a single instruction at given address
: disasm1 ( addr -- )
  dup
  [ wordbuf buf>off ] lit @ -
  w. 2 spaces
  dup c@ dup b.
  5 spaces
  disasm_insn
  nl tail
  ; noexit

public
\ disassembles many instructions at a given address
\ returns the address after the last decoded instruction
: disasm ( addr n -- addr' )
  begin
    dup if
      over disasm1
      1/str again
    then
  end
  drop
  ;

}scope

forth current !
