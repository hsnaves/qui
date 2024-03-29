\ control structures (if, then, else, etc.)
hex

\ flips the flags of the word
: toggleflags ( fl addr -- )
  dup c@
  rot xor
  swap c!
  ;

\ marks a word as immediate in the dictionary
: imm ( -- )
  F_IMM last @
  toggleflags tail
  ; noexit

\ marks a word as inline in the dictionary
: inl ( -- )
  F_INL last @
  toggleflags tail
  ; noexit

( *** basic control structures *** )
\ checks if a condition is true
: if ( -- if_addr )
  here @
  dup litja,
  I_JZ c, tail
  ; noexit imm

\ implementation of the then word
: then ( if_addr -- )
  here dup @
  rrot !
  dup litja,
  here !
  ;
last @ \ keep address of then on the stack

\ else part of if/else/then construct
: else ( if_addr -- else_addr )
  here @
  dup litja, I_JMP c,
  swap
  then tail
  ; noexit imm
F_IMM swap toggleflags \ set then to immediate

\ places the current address on the return stack
\ to later be used by words such as while, until, again, etc.
\ note the return stack currently has the caller address
\ on top, which is from the compile word
: begin ( r: rcomp -- r: label rcomp )
  r> here @  >r >r
  ; imm

\ iterates the loop if the current value on the stack is false
: until ( r: label rcomp -- r: label rcomp )
  r> r@ I_JZ jump, >r
  ; imm

\ iterates the loop regardless
: again  ( r: label rcomp -- r: label rcomp )
  r> r@ I_JMP jump, >r
  ; imm

\ iterates the loop with a call instead of a jump
: recurse ( r: label rcomp -- r: label rcomp )
   r> r@ I_JSR jump, >r
   ; imm

\ ends the begin block
: end ( r: label rcomp -- r: rcomp )
  r> rdrop >r
  ; imm

( *** miscellaneous words *** )
\ links the previous node to a given node node
: node-link ( prev node -- )
  over node>next @
  over node>next !
  swap node>next !
  ;

\ unlinks a node from the linked list
\ the previous node is passed to this word
: node-unlink ( prev -- )
  dup node>next @
  tuck node>next @
  swap node>next !
  0 swap node>next !
  ;

\ finds a node in the linked list
\ returns pointer to the previous node ( or zero if not found )
: node-find ( first node -- prev )
  begin
    over if
      over node>next @
      2dup =
      if 2drop exit then
      swap rot drop
      again
    then
  end
  drop
  ;

\ obtains the execution token of the next word
: ' ( -- xt )
  find >xt tail
  ; noexit

\ postpone the execution of an immediate word
: postpone ( -- )
  ' I_JSR jump, tail
  ; noexit imm

\ compiles a literal (immediate word)
: lit ( n -- )
  lit, tail
  ; noexit imm

\ word to create a word for a literal
: embed ( n -- )
  create lit, wrapup
  inl tail
  ; noexit

\ word to create a word for a string
: embed-str ( c-str n -- )
  create
  swap lit, lit,
  wrapup
  inl tail
  ; noexit

\ word to create a variable
: var ( size -- )
  here @ swap allot
  embed tail
  ; noexit

\ word to create a dictionary
: dictionary ( -- )
  align here @ 10 var
  0 over dict>last !
  0 over node>next !
  wordbuf over dict>code !
  wordbuf swap dict>data !
  ;

\ uses a dictionary (appends to context)
: use ( addr -- )
  context swap
  node-link tail
  ; noexit

\ use of the first dictionary (set the context)
: use-first ( addr -- )
  0 context ! use tail
  ; noexit

\ no longer uses a given dictionary
: abandon ( addr -- )
  context swap
  node-find dup
  if node-unlink tail then
  drop
  ;

\ drops the last dictionary from the context
: abandon-last ( -- )
  context node-unlink tail
  ; noexit

\ obtains the first character of the next word
: char ( -- c )
  word
  drop c@
  ;
