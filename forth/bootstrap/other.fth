\ Implementation of colon and related words
hex

\ the word to exit the interpreter
: ] ( -- )
  1 state c!
  ;

\ to start a word definition in the meta dictionary
: : ( -- addr )
  ] create tail
  ; noexit

\ the word to enter the interpreter
: [ ( -- )
  0 state c!
  ;
last @

\ to end a word definition in the meta dictionary
: ; ( addr -- )
  [ wrapup tail
  ; noexit imm
last @ swap last ! imm last ! \ make [ immediate

\ changes the last opcode from a call to a jump
: tail ( -- )
  here @
  1 - dup c@
  I_JSR = =0
  if drop exit then
  I_JMP swap c!
  ; imm

\ compiles an exit in place
: exit ( -- )
  state c@
  if exit, tail then
  ; imm

\ drop the return at the end of the word
: noexit ( -- )
  here dup @
  1 - swap  !
  ;

\ compiles a string inplace and returns the
\ counted string on the stack
: ", ( -- c-str n )
  here @
  begin
    key dup [ char " ] lit =
    if
      drop here @
      over - exit
    then
    c,
    again
  end
  ;

\ comments
: ( ( -- )
  begin
    key
    [ char ) ] lit =
    until
  end
  ; imm

\ line comments
: \ ( -- )
  line tail
  ; noexit imm

scope{
public
\ toggles the last word to immediate
: imm ( -- )
  F_IMM last @
  ; noexit \ falls through

private
\ flips the flags of the word
: toggle ( fl addr -- )
  dup c@
  rot xor
  swap c!
  ;

public
\ toggles the last word to inline
: inl ( -- )
  F_INL last @
  toggle tail
  ; noexit

}scope
