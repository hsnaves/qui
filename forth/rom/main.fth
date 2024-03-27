\ control structures (if, then, else, etc.)

hex

\ flips the flags of the word
: toggleflags ( fl addr -- )
   dup c@                       \ d: fl addr vflags
   rot xor                      \ d: addr vflags'
   swap c!                      \ d:
   ;

\ marks a word as immediate in the dictionary
: imm ( -- )
   F_IMM last @                 \ d: F_IMM vlast
   toggleflags tail
   ; noexit

\ marks a word as inline in the dictionary
: inl ( -- )
   F_INL last @                 \ d: F_INL vlast
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
   here dup @                   \ d: if_addr here vhere
   rrot !                       \ d: vhere
   dup litja,                   \ d: vhere
   here !                       \ d:
   ;
last @ \ keep address of then on the stack

\ else part of if/else/then construct
: else ( if_addr -- else_addr )
   here @                       \ d: if_addr else_addr
   dup litja, I_JMP c,          \ d: if_addr else_addr
   swap                         \ d: else_addr if_addr
   then tail                    \ d: else_addr
   ; noexit imm

F_IMM swap toggleflags \ set then to immediate

\ places the current address on the return stack
\ to later be used by words such as while, until, again, etc.
\ note the return stack currently has the caller address
\ on top, which is from the compile word
: begin ( r: rcomp -- r: label rcomp )
   r>                           \ d: rcomp | r:
   here @                       \ d: rcomp label | r:
   >r >r                        \ d: | r: label rcomp
   ; imm

\ iterates the loop if the current value on the
\ stack is false
: until ( r: label rcomp -- r: label rcomp )
   r> r@                        \ d: rcomp label | r: label
   I_JZ jump,                   \ d: rcomp | r: label
   >r                           \ d: | r: label rcomp
   ; imm

\ iterates the loop regardless
: again  ( r: label rcomp -- r: label rcomp )
   r> r@                        \ d: rcomp label | r: label
   I_JMP jump,                  \ d: rcomp | r: label
   >r                           \ d: | r: label rcomp
   ; imm

\ iterates the loop with a call instead of a jump
: recurse ( r: label rcomp -- r: label rcomp )
   r> r@                        \ d: rcomp label | r: label
   I_JSR jump,                  \ d: rcomp | r: label
   >r                           \ d: | r: label rcomp
   ; imm

\ ends the begin block
: end ( r: label rcomp -- r: rcomp )
   r>                           \ d: rcomp | r: label
   rdrop                        \ d: rcomp | r:
   >r                           \ d: | r: rlabel
   ; imm

( *** miscellaneous words *** )

\ links the previous node to a given node node
: node-link ( prev node -- )
   over node>next @             \ d: prev node prevnext
   over node>next !             \ d: prev node
   swap node>next !             \ d:
   ;

\ unlinks a node from the linked list
\ the previous node is passed to this word
: node-unlink ( prev -- )
   dup node>next @              \ d: prev curr
   tuck node>next @             \ d: curr prev next
   swap node>next !             \ d: curr
   0 swap node>next !           \ d:
   ;

\ finds a node in the linked list
\ returns pointer to the previous node ( or zero if not found )
: node-find ( first node -- prev )
   begin
      over if
         over node>next @       \ d: curr node next
         2dup =                 \ d: curr node next eq?
         if 2drop exit then     \ return curr

         swap rot drop          \ d: curr' node
         again
      then
   end
   drop
   ;

\ word to create a variable
: var ( size -- )
   here @                       \ d: size vhere
   swap allot                   \ d: vhere
   create                       \ d: vhere
   lit,                         \ d:
   wrapup
   inl
   ;

\ word to create a dictionary
: dictionary ( -- )
   align here @ 10 var          \ d: addr
   0 over dict>last !           \ d: addr
   0 over node>next !           \ d: addr
   wordbuf over dict>code !     \ d: addr
   wordbuf swap dict>data !     \ d: addr
   ;

\ uses a dictionary (appends to context)
: use ( addr -- )
   context swap                 \ d: context addr
   node-link tail
   ; noexit

\ use of the first dictionary (set the context)
: use-first ( addr -- )
   0 context ! use tail
   ; noexit

\ no longer uses a given dictionary
: abandon ( addr -- )
   context swap                 \ d: context addr
   node-find dup                \ d: prev prev
   if node-unlink tail then
   drop
   ;

\ drops the last dictionary from the context
: abandon-last ( -- )
   context node-unlink tail
   ; noexit

\ obtains the execution token of the next word
: ' ( -- xt )
   find >xt tail               \ d: xt
   ; noexit

\ obtains the first character of the next word
: char ( -- c )
   word                         \ d: c-str n
   drop c@                      \ d: c
   ;

\ compiles a literal (immediate word)
: lit ( n -- )
   lit, tail                    \ d:
   ; noexit imm

