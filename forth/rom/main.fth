\ control structures (if, then, else, etc.)
hex

( *** basic control structures *** )
\ checks if a condition is true
: if ( -- if_addr )
  here @
  dup litj,
  I_JZ c, tail
  ; noexit imm

\ implementation of the then word
: then ( if_addr -- )
  here dup @
  rot rot !
  dup litj,
  here !
  ;
last @ \ keep address of then on the stack

\ else part of if/else/then construct
: else ( if_addr -- else_addr )
  here @
  dup litj, I_JMP c,
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
  1 r@ I_JZ jump, tail
  ; noexit imm

\ iterates the loop regardless
: again  ( r: label rcomp -- r: label rcomp )
  1 r@ I_JMP jump, tail
  ; noexit imm

\ iterates the loop with a call instead of a jump
: recurse ( r: label rcomp -- r: label rcomp )
   1 r@ I_JSR jump, tail
   ; noexit imm

\ ends the begin block
: end ( r: label rcomp -- r: rcomp )
  r> 0 r!
  ; imm

( *** miscellaneous words *** )

\ detect if the meta compiler is present
\ the stack will contain the address of the internal
\ dictionary or the meta dictionary
word meta 0 lookup nip nip =0
dup internal and swap =0 current @ and or

current @ over current !

\ links the previous node to a given node node
: node-link ( prev node -- )
  over node>next @
  over node>next !
  swap node>next !
  ;

\ unlinks a node from the linked list
\ the previous node is passed to this word
: node-drop ( prev -- )
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
      if drop drop exit then
      swap rot drop
      again
    then
  end
  drop
  ;
current !

\ obtains the execution token of the next word
: ' ( -- xt )
  find >xt tail
  ; noexit

\ compiles a literal (immediate word)
: lit ( n -- )
  lit, tail
  ; noexit imm

: " ( -- c-str n )
  state c@ =0 if ", tail then
  here @ dup litj, I_JSR c, ", swap drop
  swap here @ swap here ! dup litj, here !
  [ ' r> c@ ] lit c, lit, tail
  ; noexit imm

\ uses a dictionary (appends to context)
: use ( addr -- )
  context swap
  node-link tail
  ; noexit

\ no longer uses a given dictionary
: discard ( addr -- )
  context swap
  node-find dup
  if node-drop tail then
  drop
  ;

drop
