\ control structures (if, then, else, etc.)
hex

( *** basic control structures *** )
\ checks if a condition is true
: if ( -- if_addr )
  here @
  dup lj,
  JZ c, tail
  ; noexit imm

\ implementation of the then word
: then ( if_addr -- )
  here dup @
  rot rot !
  dup lj,
  here !
  ;
last @ \ keep address of then on the stack

\ else part of if/else/then construct
: else ( if_addr -- else_addr )
  here @
  dup lj, JMP c,
  swap
  then tail
  ; noexit imm
last @ swap last ! imm last ! \ set then to immediate

\ places the current address on the return stack
\ to later be used by words such as while, until, again, etc.
\ note the return stack currently has the caller address
\ on top, which is from the compile word
: begin ( r: rcomp -- r: label rcomp )
  r> here @  >r >r
  ; imm

\ iterates the loop if the current value on the stack is false
: until ( r: label rcomp -- r: label rcomp )
  1 r@ JZ j, tail
  ; noexit imm

\ iterates the loop regardless
: again  ( r: label rcomp -- r: label rcomp )
  1 r@ JMP j, tail
  ; noexit imm

\ iterates the loop with a call instead of a jump
: recurse ( r: label rcomp -- r: label rcomp )
   1 r@ JSR j, tail
   ; noexit imm

\ ends the begin block
: end ( r: label rcomp -- r: rcomp )
  r> 0 r!
  ; imm

( *** miscellaneous words *** )

\ compiles a literal (immediate word)
: lit ( n -- )
  l, tail
  ; noexit imm

\ obtains the execution token of the next word
: ' ( -- xt )
  find >xt tail
  ; noexit

: " ( -- c-str n )
  state c@ =0 if ", tail then
  here @ dup lj, JSR c, ", swap drop
  swap here @ swap here ! dup lj, here !
  [ ' r> c@ ] lit c, l, tail
  ; noexit imm

\ uses a dictionary (appends to context)
: use ( addr -- )
  context swap
  over node>next @
  over node>next !
  swap node>next !
  ;

\ no longer uses a given dictionary
: scrap ( addr -- )
  context swap

  begin
    over =0 if drop drop exit then
    over node>next @
    2dup = =0
    if swap rot drop again then
  end
  drop drop

  dup node>next @
  tuck node>next @
  swap node>next !
  0 swap node>next !
  ;
