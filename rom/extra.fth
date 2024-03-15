( words to change the current value of base )

\ change to hexadecimal base
: hex ( -- )
   16 base c!
   ;

\ change to decimal base
: decimal ( -- )
   10 base c!
   ;

\ every literal is now in hexadecimal base
hex

( auxiliary words for changing the flags of words )
\ drop the return at the end of the word
: noexit ( -- )
   code buf>here dup @
   1- swap  !
   ;

\ sets a flag of the word
: setflag ( fl addr -- )
   dup c@                       \ d: fl flags vflags
   rot                          \ d: flags vflags fl
   E0 and or                    \ d: flags vflags'
   swap c!                      \ d:
   ;

\ marks a word as immediate in the dictionary
: imm ( -- )
   40 last @                    \ d: F_IMM vlast
   setflag tail
   ; noexit

\ marks a word as inline in the dictionary
: inl ( -- )
   20 last @                    \ d: F_INL vlast
   setflag tail
   ; noexit

\ set the compiler as the current dictionary
compiler current !

( define constants for flags )
: F_EXT    80 ; inl
: F_IMM    40 ; inl
: F_INL    20 ; inl
: F_LINK   10 ; inl
: F_XT     08 ; inl

( define constants for useful instructions )
: insn_ret C0 ; inl
: insn_jsr C1 ; inl
: insn_jmp C2 ; inl
: insn_jz  C3 ; inl

\ set the forth as the current dictionary
forth current !

( very useful Forth words )

\ the word to enter the interpreter
: [ ( -- )
   0 state c!                   \ set the state to 0
   ; imm

\ the word to exit the interpreter
: ] ( -- )
   1 state c!                   \ set the state to 1
   ;

\ basic control structures
: if ( -- if_addr )
   here @
   0 2 litn,
   insn_jz c, tail
   ; noexit imm

\ implementation of the then
: then ( if_addr -- )
   dup                          \ d: if_addr if_addr
   here dup @                   \ d: if_addr if_addr here vhere
   over >r >r                   \ d: if_addr if_addr here | r: here vhere
   !                            \ d: if_addr | r: here vhere
   r@ swap -                    \ d: diff | r: here vhere
   3 -                          \ d: diff' | r: here vhere
   2 litn,                      \ d: | r: here vhere
   r> r> !                      \ d:
   ;
\ then is not marked as immediate yet
last @

\ else part of if/else/then construct
: else ( if_addr -- else_addr )
   here @                       \ d: if_addr else_addr
   0 2 litn, insn_jmp c,        \ d: if_addr else_addr
   swap                         \ d: else_addr if_addr
   then tail                    \ d: else_addr
   ; noexit imm

\ set the then word to immediate
F_IMM swap setflag

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
   insn_jz jump,                \ d: rcomp | r: label
   >r                           \ d: | r: label rcomp
   ; imm

\ iterates the loop regardless
: again  ( r: label rcomp -- r: label rcomp )
   r> r@                        \ d: rcomp label | r: label
   insn_jmp jump,               \ d: rcomp | r: label
   >r                           \ d: | r: label rcomp
   ; imm

\ iterates the loop with a call instead of a jump
: recurse ( r: label rcomp -- r: label rcomp )
   r> r@                        \ d: rcomp label | r: label
   insn_jsr jump,               \ d: rcomp | r: label
   >r                           \ d: | r: label rcomp
   ; imm

\ ends the begin block
: end ( r: label rcomp -- r: rcomp )
   r>                           \ d: rcomp | r: label
   r> drop                      \ d: rcomp | r:
   >r                           \ d: | r: rlabel
   ; imm

\ obtains the execution token of the next word
: ' ( -- xt )
   word 2dup 0 lookup           \ d: c-str n addr
   dup =0                       \ d: c-str n addr zero?
   if drop unknown tail then
   nip nip >xt tail             \ d: xt
   ; noexit

\ obtains the first character of the next word
: char ( -- c )
   word                         \ d: c-str n
   drop c@
   ;

\ set the compiler as the current dictionary
compiler current !

\ compiles a string inplace and returns the
\ counted string on the stack
: ," ( -- c-str n )
   here @                       \ d: c-str
   begin
      key                       \ d: c-str c
      dup [ char " lit, ] =     \ d: c-str c end?
      if                        \ d: c-str c
         drop                   \ d: c-str
         here @                 \ d: c-str here
         over - exit            \ d: c-str n
      then
      c,                        \ d: c-str
      again
   end
   ;

\ allocates a given number of bytes
: allot ( n -- )
   here 2dup %overflow?         \ d: n here
   swap over @                  \ d: here n vhere
   + swap !                     \ d:
   ;

\ aligns the here pointer to multiple of a cell
: align ( -- )
   here @                       \ d: vhere
   dup 3 +                      \ d: vhere vhere'
   dup 3 and                    \ d: vhere vhere' rem
   - swap -                     \ d: n
   allot tail
   ; noexit

\ set the forth as the current dictionary
forth current !

\ word to create a variable
: var ( size -- )
   here @                       \ d: size vhere
   swap allot                   \ d: vhere
   create                       \ d: vhere
   lit,                         \ d:
   wrapup
   inl
   ;

\ creates a deferred word
: defer ( -- )
   here @                       \ d: addr
   0 ,                          \ d: addr
   create                       \ d: addr
   lit,                         \ d:
   [ ' @ c@ lit, ]              \ get the xt of @
   c,                           \ compile "@"
   [ ' exec lit, ]              \ get the xt of exec
   insn_jmp jump,               \ compile jump to exec
   wrapup
   ;

\ sets the xt of a deferred word
: is ( xt -- )
   word 0 lookup                \ d: xt addr
   4 - !
   ;

\ word to create a dictionary
: dictionary ( -- )
   align
   here @ 10 var                \ d: addr
   0 over dict>last !           \ d: addr
   0 over dict>next !           \ d: addr
   wordbuf over dict>code !     \ d: addr
   wordbuf over dict>data !     \ d: addr
   drop                         \ d:
   ;

\ uses a dictionary
: use ( addr -- )
   context @                    \ d: addr vcontext
   over dict>next !             \ d: addr
   context !                    \ d:
   ;

\ uses only forth in the context
: forthonly
   0 [ forth dict>next lit, ] !
   forth context !
   ;

( implementation of the private / public framework )

\ initialize the tempbuf and temp
70 buf>start @ 70 buf>here !
00 60 dict>last !
70 60 dict>code !
70 60 dict>data !
compiler 1C !

\ set the current to temp
60 use
60 current !

( define the location of temporary buffer and temporary dictionary )
: temp ( -- addr )     60 ; inl
: tmpbuf ( -- addr )   70 ; inl
: tempref ( -- addr )  1C ; inl

forth current !

\ start the scope
: scope{ ( -- )
   \ set here to start
   [ tmpbuf buf>start lit, ] @
   [ tmpbuf buf>here lit, ] !

   \ set last word to zero
   0 [ temp dict>last lit, ] !  \ d:

   \ clear the tempref
   0 tempref !

   \ set temp code to tmpbuf
   tmpbuf [ temp dict>code lit, ] !

   \ set temp data to tmpbuf
   tmpbuf [ temp dict>code lit, ] !

   \ set the context to the temp
   temp use
   ;

\ the public declarations of the scope
: public ( -- )
   current @                    \ d: vcurrent
   temp =                       \ d: eq?
   if
      tempref @ current !
   then
   ;

\ the private declarations of the scope
: private ( -- )
   public                       \ d:
   current @                    \ d: vcurrent
   dup tempref !                \ d: vcurrent
   temp current !               \ d: vcurrent
   dict>code @                  \ d: currentbuf
   [ temp dict>code lit, ] !    \ d:
   ;

\ auxiliary declarations in the scope
: auxiliary ( -- )
   private                      \ d:
   tmpbuf                       \ d: tmpbuf
   [ temp dict>code lit, ] !    \ d:
   ;

\ stop the scope
: }scope ( -- )
   public
   [ temp dict>next lit, ] @    \ d: vcontext
   context !                    \ d:
   0 [ temp dict>next lit, ] !  \ d:
   0 tempref !
   ;

}scope

scope{
private

\ digit to character word
: d>c ( dig -- c )
   dup 0A u<                    \ d: dig less10?
   =0 if                        \ d: dig
      [ char A 0A - lit, ]
      +
      exit
   then
   [ char 0 lit, ] +            \ d: c
   ;

\ prints a digit to the output ( no space at end )
: d. ( dig --)
   d>c emit tail                \ d:
   ; noexit

public

\ prints a byte to the output in hexadecimal (no space)
: b. ( b -- )
   dup 04 ushr                  \ d: b {b>>4}
   0F and d.                    \ d: b
   0F and d. tail               \ d:
   ; noexit

\ prints a half-word to the output in hexadecimal (no space)
: h. ( h -- )
   dup 08 ushr                  \ d: h {h>>8}
   b. b. tail                   \ d:
   ; noexit

\ prints a word to the output in hexadecimal (no space)
: w. ( w -- )
   dup 10 ushr                  \ d: w {w>>16}
   h. h. tail                   \ d:
   ; noexit

\ prints an unsigned word to the output in the current
\ base (no space at end)
: u. ( u -- )
   begin                        \ d: u
      base c@ u/mod             \ d: rem quot
      dup =0 if                 \ d: rem quot
         drop                   \ d: rem
         d. tail                \ d:
      then                      \ d: rem quot
      recurse                   \ d: rem
   end                          \ d: rem
   d>c emit tail                \ d:
   ; noexit

\ prints a signed integer to the output in the current
\ base (no space at end)
: . ( num -- )
   dup                          \ d: num num
   0 <                          \ d: num neg?
   if
      [ char - lit, ] emit      \ print minus sign
      0 swap -                  \ d: -num
   then
   u. tail                      \ d:
   ; noexit

\ prints a given number of spaces
: spaces ( n -- )
   begin
      dup if
         space 1-
         again
      then
   end
   drop
   ;

private

\ prints the words in a given dictionary
: words1 ( dict -- )
   dict>last @
   begin
      dup
      =0 if drop exit then
      dup >name type space
      >link
      again
   end
   ;

public

\ prints the words in the context
: words ( -- )
   context @
   begin
      dup
      =0 if drop exit then
      dup words1 nl nl
      dict>next @
      again
   end
   ;

private

( words for handling exceptions )

\ clears the exception bit
: clearexception ( -- )
   0 -14 !
   ;

\ handles general exceptios
: general_exception ( status c-str n -- )
   1 channel c!                 \ d: status c-str n
   type                         \ d: status
   drop r>                      \ d: pc
   dup w. space                 \ d: pc
   c@ b. nl                     \ d:
   0 channel c!
   clearexception
   quit tail
   ; noexit

," invalid instruction at "

\ handles invalid instructions
: invalid_insn ( status -- )
   [ swap lit, lit, ]
   general_exception tail
   ; noexit

," divide by zero at "

\ handles divide by zero errors
: divide_by_zero ( status -- )
   [ swap lit, lit, ]
   general_exception tail
   ; noexit

," stack overflow at "
: stack_overflow ( pc status -- )
   1 channel c!                 \ d: pc status | r: newpc
   [ swap lit, lit, ]           \ d: pc status c-str n | r: newpc
   type                         \ d: pc status | r: newpc
   drop                         \ d: pc | r: newpc
   dup w. space                 \ d: pc | r: newpc
   c@ b. space                  \ d: | r: newpc
   r>                           \ d: newpc
   w. nl                        \ d:
   0 channel c!
   1 dsp! 0 rsp!
   clearexception
   quit tail
   ;

\ current implementation of onexception
: my_onexception ( status -- )
   dup 1 = if
      invalid_insn tail
   then
   dup 2 = if
      divide_by_zero tail
   then
   dup 3 = if
      stack_overflow tail
   then
   bye tail
   ; noexit

' my_onexception is onexception

}scope

decimal

( dump rom file to standard output )
0 here @ type

0 bye
