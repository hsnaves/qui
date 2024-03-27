\ Implementation of colon and related words

hex

\ to start a word definition in the meta dictionary
: : ( -- addr )
   create                       \ call the word creation
   1 state c!                   \ set the state to 1
   ;


\ to end a word definition in the meta dictionary
: ; ( addr -- )
   0 state c!                   \ set the state to 0
   wrapup tail
   ; noexit imm

\ the word to enter the interpreter
: [ ( -- )
   0 state c!                   \ set the state to 0
   ; imm

\ the word to exit the interpreter
: ] ( -- )
   1 state c!                   \ set the state to 1
   ;

\ changes the last opcode from a call to a jump
: tail ( -- )
   here @                       \ d: vhere
   1- dup c@                    \ d: vhere' c
   I_JSR <>                     \ d: vhere' notcall?
   if drop exit then
   I_JMP swap c!                \ d:
   ; imm

\ compiles an exit in place
: exit ( -- )
   state c@
   if exit, tail then
   ; imm

\ drop the return at the end of the word
: noexit ( -- )
   here dup @                   \ d: here vhere
   1- swap  !                   \ d:
   ;

\ comments
: ( ( -- )
   begin
      key                       \ d: c
      [ char ) ] lit =          \ d: end?
      until                     \ d:
   end
   ; imm

\ line comments
: \ ( -- )
   line tail
   ; noexit imm
