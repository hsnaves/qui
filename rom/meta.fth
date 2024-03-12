hex


dictionary meta
context @ meta dict>next !
meta current !
meta context !

scope{
public

align
4 var moffset

( define constants )
: m:F_EXT    80 ; inl
: m:F_IMM    40 ; inl
: m:F_INL    20 ; inl
: m:F_LINK   10 ; inl
: m:F_XT     08 ; inl

: m:current  00 ; inl
: m:context  04 ; inl
: m:this     08 ; inl
: m:flags    0C ; inl
: m:state    0D ; inl
: m:base     0E ; inl
: m:forth    10 ; inl
: m:compiler 20 ; inl
: m:wordbuf  30 ; inl
: m:extrabuf 40 ; inl
: m:tib      50 ; inl
: m:temp     60 ; inl
: m:tmpbuf   70 ; inl

: m:insn_ret C0 ; inl
: m:insn_jsr C1 ; inl
: m:insn_jmp C2 ; inl
: m:insn_jz  C3 ; inl

( conversion to meta and host words )
\ host address to meta address
: m:addr>meta ( addr -- addr' )
   moffset @ -
   ; inl

\ meta address to host address
: m:addr>host ( addr -- addr' )
   moffset @ +
   ; inl

( read / write in meta system )
\ read cell in meta system
: m:@ ( addr -- v )
   m:addr>host @
   ;

\ write cell in meta system
: m:! ( v addr -- )
   m:addr>host !
   ;

\ read byte in meta system
: m:c@ ( addr -- v )
   m:addr>host c@
   ;

\ write byte in meta system
: m:c! ( v addr -- )
   m:addr>host c!
   ;

( words for accessing parts of dictionary )
\ last word of dictionary
: m:dict>last ( addr -- addr )
   ; inl

\ next dictionary (sibling)
: m:dict>next ( addr -- addr )
   04 +
   ; inl

\ buffer for code in dictinary
: m:dict>code ( addr -- addr )
   08 +
   ; inl

\ buffer for data in dictionary
: m:dict>data ( addr -- addr )
   0C +
   ; inl

( words for accessing parts of the buffer )
\ current position in buffer
: m:buf>here ( addr -- addr )
   ; inl

\ start of the buffer
: m:buf>start ( addr -- addr )
   04 +
   ; inl

\ end of the buffer
: m:buf>end ( addr -- addr )
   08 +
   ; inl

\ extra offset (for relative address or TIB position)
: m:buf>off ( addr -- addr )
   0C +
   ; inl

( useful words for compilation )
\ the code buffer
: m:code ( -- addr )
   m:current m:@
   m:dict>code m:@ tail
   ; noexit

\ the data buffer
: m:data ( -- addr )
   m:current m:@
   m:dict>data m:@ tail
   ; noexit

\ position of last word in the current dictionary
: m:last ( -- addr )
   m:current m:@
   m:dict>last
   ;

\ current position of the code buffer
: m:here  ( -- addr )
   m:code m:buf>here
   ;

private
," buffer overflow"
: m:overflow ( -- )
   [ swap lit, lit, ]           \ d: c-str n
   onerror tail
   ; noexit

public

\ detects if the buffer will overflow
: m:%overflow? ( n here -- )
   dup m:buf>end m:@            \ d: n here vend
   swap m:buf>start m:@         \ d: n vend vstart
   - swap                       \ d: rem n
   u<                           \ d: overflow?
   if m:overflow tail then
   ;

\ compile a value in a user-selected buffer
: m:%, ( v here -- )
   4 over m:%overflow?          \ d: v here
   tuck                         \ d: here v here
   m:@ tuck m:!                 \ d: here vhere
   4 + swap m:! tail            \ d:
   ; noexit

\ compile a byte in a user-selected buffer
: m:%c, ( b here -- )
   1 over m:%overflow?          \ d: v here
   tuck                         \ d: here b here
   m:@ tuck m:c!                \ d: vhere
   1+ swap m:! tail             \ d:
   ; noexit

\ compiles a string in a user-selected buffer
: m:%str, ( c-str n here -- )
   >r                           \ d: c-str n | r: here
   begin                        \ d: c-str n | r: here
      dup if                    \ d: c-str n | r: here
         over c@                \ d: c-str n c | r: here
         r@ m:%c,               \ d: c-str n | r: here
         str1+                  \ d: c-str' n' | r: here
         again
      then                      \ d: c-str 0 | r: here
   end
   2drop                        \ d: | r: here
   r> drop                      \ d:
   ;

\ compile a value in the code buffer
: m:, ( v -- )
   m:here                       \ d: v here
   m:%, tail
   ; noexit

\ compile a byte in the code buffer
: m:c, ( b -- )
   m:here                       \ d: b here
   m:%c, tail
   ; noexit

\ compiles a string in the code buffer
: m:str, ( c-str n -- )
   m:here                       \ d: c-str n here
   m:%str, tail
   ; noexit

\ compiles a string from the text in memory
: m:," ( -- c-str n )
   m:here m:@                   \ d: c-str
   begin
      key                       \ d: c-str c
      dup [ char " lit, ] =     \ d: c-str c end?
      if                        \ d: c-str c
         drop                   \ d: c-str
         m:here m:@             \ d: c-str here
         over - exit            \ d: c-str n
      then
      m:c,                      \ d: c-str
      again
   end
   ;

\ compile a sequence of instructions inline
: m:inline, ( addr -- )
   begin
      dup m:c@                  \ d: addr c
      dup m:insn_ret =          \ d: addr c ret?
      =0 if m:c, 1+ again then
   end
   2drop                        \ d:
   ;

\ compile a literal
: m:lit, ( v -- )
   begin
      dup dup                   \ d: v v v
      6 sge =                   \ d: v se6?
      if
         3F and 80 +            \ d: v'
         m:c, exit              \ d:
      then
      dup 7 shr                 \ d: v {v>>7}
      recurse                   \ d: v
      7F and m:c, tail          \ d:
   end
   ; noexit

\ compile a literal of a given fixed size
: m:litn, ( v n -- )
   begin
      1- dup                    \ d: v n' n'
      =0 if
         drop                   \ d: v
         3F and 80 +            \ d: v'
         m:c, exit
      then
      over 7 shr                \ d: v n {v>>7}
      swap recurse              \ d: v
      7F and m:c, tail          \ d:
   end
   ; noexit

\ checks if a given value fits a literal of fixed size
: m:litn? ( v n -- b )
   7 * 1-                       \ d: v bits
   over swap                    \ d: v v bits
   sge =                        \ d: b
   ;

\ compiles a jump to a target
: m:jump, ( target insn -- )
   >r                           \ d: target | r: insn
   m:here m:@                   \ d: target vhere | r: insn
   1+ -                         \ d: diff | r: insn
   1                            \ d: diff n | r: insn
   begin
      2dup -                    \ d: diff n v | r: insn
      over m:litn?              \ d: diff n ok? | r: insn
      =0 if 1+ again then
   end
   swap over -                  \ d: n diff' | r: insn
   swap m:litn,                 \ d: | r: insn
   r> m:c, tail                 \ d:
   ; noexit

\ compiles a jump of a given fixed size
: m:jumpn, ( target n insn -- )
   >r >r                        \ d: target | r: insn n
   m:here m:@                   \ d: target vhere | r: insn n
   r@ 1+ + -                    \ d: v | r: insn n
   r> m:litn,                   \ d: | r: insn
   r> m:c, tail                 \ d:
   ; noexit

\ compiles a return
: m:exit, ( -- )
   m:insn_ret m:c, tail         \ compile return
   ; noexit

\ compiles a `>r`
: m:>r, ( -- )
   DD m:c, tail
   ; noexit

\ compiles a `r>`
: m:r>, ( -- )
   DE m:c, tail
   ; noexit

\ compiles a `r@`
: m:r@, ( -- )
   DF m:c, tail
   ; noexit

\ allocates a given number of bytes
: m:allot ( n -- )
   m:here 2dup m:%overflow?     \ d: n here
   swap over m:@                \ d: here n vhere
   + swap m:! tail              \ d:
   ; noexit

\ aligns the here pointer to multiple of a cell
: m:align ( -- )
   m:here m:@                   \ d: vhere
   dup 3 +                      \ d: vhere vhere'
   dup 3 and                    \ d: vhere vhere' rem
   - swap -                     \ d: n
   m:allot tail
   ; noexit

( words for accessing parts of a word in the dictionary )
\ obtains the flags for the word
: m:>flags ( addr -- fl )
   m:c@                         \ d: fl
   dup m:F_EXT and              \ d: fl ext?
   =0 if
      [
         m:F_EXT
         m:F_IMM or
         m:F_INL or
         lit,
      ]                         \ d: fl mask
      and                       \ d: fl
   then
   ;

\ obtains the link to the next word
: m:>link ( addr -- addr' )
   dup m:>flags                 \ d: addr fl
   over 1+                      \ d: addr fl addr'

   over m:F_EXT and             \ d: addr fl addr' ext?
   if 1+ then                   \ d: addr fl addr'

   over m:F_XT and              \ d: addr fl addr' xt?
   if 4 + then                  \ d: addr fl addr'

   swap m:F_LINK and            \ d: addr addr' link?
   =0 if
      m:c@ sge8
   else
      m:@
   then                         \ d: addr diff
   dup =0 if                    \ d: addr diff
      nip exit
   then
   +                            \ d: addr'
   ;

\ obtains the name of the word
: m:>name ( addr -- c-addr n )
   dup m:c@                     \ d: addr fl
   >r 1+                        \ d: addr' | r: fl
   r@ m:F_EXT and               \ d: addr ext? | r: fl
   if
      dup 1+                    \ d: addr addr+1 | r: fl
      swap m:c@                 \ d: addr' len | r: fl
   else
      r@ 1F and                 \ d: addr len | r: fl
      r> E0 and >r              \ d: addr len | r: fl
   then
   swap                         \ d: len addr | r: fl
   r@ m:F_XT and                \ d: len addr xt? | r: fl
   if 4 + then                  \ d: len addr' | r: fl
   1+ r> m:F_LINK and           \ d: len addr' link?
   if 3 + then                  \ d: len addr'
   swap                         \ d: c-str len
   ;

\ obtains the address of the first instruction
\ of the word
: m:>xt ( addr -- addr )
   dup m:>flags                 \ d: addr fl
   m:F_XT and                   \ d: addr xt?
   if 2 + m:@ exit then
   m:>name +                    \ d: addr'
   ;

private

\ finds a word in the dictionary at given address
: m:lookup1 ( c-str n dict -- addr )
   swap >r swap >r              \ d: dict | r: n c-str
   m:dict>last m:@              \ d: addr | r: n c-str
   begin
       dup if                   \ d: addr | r: n c-str
          dup m:>name           \ d: addr c-str2 n2 | r: n c-str
          swap m:addr>host swap \ d: addr c-str2' n2 | r: n c-str
          r> r@ over >r         \ d: addr c-str2 n2 c-str n | r: n c-str
          compare               \ d: addr eq | r: n c-str
          if
              m:>link           \ d: addr' | r: n c-str
              again
          then
       then
       r> r> 2drop              \ d: addr
   end
   ;

\ finds a word in the all the sibling dictionaries
: m:lookupn ( c-str n dict -- addr )
   begin
      dup if                    \ d: c-str n dict
         >r                     \ d: c-str n | r: dict
         2dup r@ m:lookup1      \ d: c-str n addr | r: dict
         dup if
            nip nip             \ d: addr | r: dict
            r> drop exit        \ d: addr
         then
         drop r>                \ d: c-str n dict
         m:dict>next m:@        \ d: c-str n dict
         again
      then
   end
   nip nip                      \ d: 0
   ;

public

\  finds a word in the context or in the current dictionary
: m:lookup ( c-str n skip? -- addr )
   >r 2dup                      \ d: c-str n c-str n | r: skip?
   m:context m:@ m:lookupn      \ d: c-str n addr | r: skip?
   r> over or                   \ d: c-str n addr skip?
   if
      nip nip exit              \ d: addr
   then
   drop m:current m:@           \ d: c-str n dict
   m:lookup1 tail               \ d: addr
   ; noexit

private

\ computes the link to the last word for a given word address
: m:link ( addr -- diff )
   m:last m:@                   \ d: addr vlast
   tuck swap - swap             \ d: diff vlast
   bool and                     \ d: diff'
   ;

\ updates the flags according to the word length and
\ the current state of the buffers and the position of the
\ last word
\ returns the updated flags and the merged first byte
: m:updateflags ( fl n -- fl' fb )
   >r                           \ d: fl | r: n

   \ check for large word length
   1F r@ u<                     \ d: fl large? | r: n
   if m:F_EXT or then           \ d: fl' | r: n

   \ check if the code buffer is the same as the data buffer
   m:data m:buf>here >r         \ d: fl | r: n here
   m:here r@ =                  \ d: fl same? | r: n here
   =0 if m:F_XT or then         \ d: fl' | r: n here

   \ check for large link
   r> m:@                       \ d: fl vhere | r: n
   m:link                       \ d: fl diff | r: n
   dup sge8 =                   \ d: fl short? | r: n
   =0 if m:F_LINK or then       \ d: fl' | r: n

   \ ensure that F_EXT is present when extra flags are set
   dup 1F and                   \ d: fl extra? | r: n
   if m:F_EXT or then           \ d: fl' | r: n

   \ merge in the length when F_EXT is not set
   dup dup m:F_EXT and          \ d: fl fb ext? | r: n
   =0 if r@ or then             \ d: fl fb'  | r: n
   r> drop                      \ d: fl fb
   ;

public

\ creates a word in the current meta dictionary
: m:create ( -- addr )
   m:flags m:c@                 \ d: fl
   word swap                    \ d: fl n c-str
   >r >r                        \ d: fl | r: c-str n
   r@ m:updateflags             \ d: fl' fb | r: c-str n
   m:data m:buf>here            \ d: fl fb here | r: c-str n
   dup m:@ m:this m:!           \ set the this pointer

   tuck m:%c,                   \ d: fl here | r: c-str n
   over m:F_EXT and             \ d: fl here ext? | r: c-str n
   if r@ over m:%c, then        \ d: fl here | r: c-str n

   over m:F_XT and              \ d: fl here xt? | r: c-str n
   if 0 over m:%, then          \ d: fl here | r: c-str n
                                \ the xt is to be populated later

   2dup                         \ d: fl here fl here | r: c-str n
   m:this m:@                   \ d: fl here fl here vthis | r: c-str n
   m:link                       \ d: fl here fl here diff | r: c-str n
   swap rot m:F_LINK and        \ d: fl here diff here link? | r: c-str n
   if m:%, else m:%c, then      \ d: fl here | r: c-str n

   r> r> swap                   \ d: fl here c-str n
   rot m:%str,                  \ d: fl
   m:F_XT and                   \ d: xt?
   if
      m:here m:@                \ d: vcodehere
      m:this m:@ 2 +            \ d: vcodehere xt-addr
      m:!                       \ d:
   then
   ;

\ to end a word definition in the meta dictionary
: m:wrapup ( addr -- )
   m:exit,                      \ compile return
   m:this m:@ m:last m:!        \ set last to this
   0 m:this m:!                 \ set list to zero
   0 m:state m:c! tail          \ set the state to 0
   ; noexit

\ interprets a single word ( given as counted string )
: m:interpret ( c-str n -- )
   2dup state c@ =0 lookup      \ d: c-str n addr
   dup if
      dup >xt                   \ d: c-str n addr xt
      swap >flags               \ d: c-str n xt flags
      F_IMM and                 \ d: c-str n xt imm?
      state c@ =0 or            \ d: c-str n xt imm?'
      if
         nip nip                \ d: xt
         exec tail
      then
   then
   drop

   2dup 0 m:lookup              \ d: c-str n addr
   dup if                       \ d: c-str n addr
      dup m:>xt                 \ d: c-str n addr xt
      swap m:>flags             \ d: c-str n xt flags
      m:F_INL and               \ d: c-str n xt inline?
      if                        \ d: c-str n xt
         m:inline,              \ d: c-str n
      else
         m:insn_jsr m:jump,     \ d: c-str n
      then                      \ d: c-str n
      2drop                     \ d:
      exit
   then

   drop                         \ d: c-str n
   2dup number                  \ d: c-str n num error?
   if
      drop unknown tail
   then
   nip nip                      \ d: num
   state c@
   if m:lit, then
   ;

\ meta Forth interpreter
\ it interprets all the words in an infinite loop
\ except when the m:state turns to zero
: m:interpreter ( -- )
   begin                        \ d:
      m:state m:c@              \ d: vstate
      =0 if exit then
      word                      \ d: c-str n
      m:interpret               \ d: ...
      again
   end
   ;

\ terminate the compilation of a word
: m:;
   0 state c!
   m:wrapup tail
   ; noexit imm

\ to start a word definition in the meta dictionary
: m:: ( -- )
   m:create                     \ d:
   1 state c!                   \ set the state to 1
   1 m:state m:c!               \ set the m:state to 1
   m:interpreter tail
   ; noexit

: m:setflag ( fl addr -- )
   dup m:c@                     \ d: fl addr vflags
   rot                          \ d: flags vflags fl
   E0 and or                    \ d: flags vflags'
   swap m:c! tail               \ d:
   ; noexit

\ marks a word as immediate in the meta dictionary
: m:imm ( -- )
   m:F_IMM m:last m:@
   m:setflag tail
   ; noexit

\ marks a word as inline in the meta dictionary
: m:inl ( -- )
   m:F_INL m:last m:@
   m:setflag tail
   ; noexit

\ changes the last opcode from a call to a jump
: m:tail ( -- )
   m:here m:@                   \ d: vhere
   1- dup m:c@                  \ d: vhere' c
   m:insn_jsr =                 \ d: vhere' call?
   =0 if drop exit then
   m:insn_jmp swap m:c! tail    \ d:
   ; noexit imm

: m:noexit ( -- )
   m:here dup m:@               \ d: here vhere
   1- swap m:! tail             \ d:
   ; noexit

\ basic control structures
: m:if ( -- if_addr )
   m:here m:@                   \ d: if_addr
   0 2 m:litn,                  \ d: if_addr
   m:insn_jz m:c, tail          \ d: if_addr
   ; noexit imm

\ implementation of the then
: m:then ( if_addr -- )
   dup                          \ d: if_addr if_addr
   m:here dup m:@               \ d: if_addr if_addr here vhere
   over >r >r                   \ d: if_addr if_addr here | r: here vhere
   m:!                          \ d: if_addr | r: here vhere
   r@ swap -                    \ d: diff | r: here vhere
   3 -                          \ d: diff' | r: here vhere
   2 m:litn,                    \ d: | r: here vhere
   r> r> m:! tail               \ d:
   ; noexit
\ then is not marked as immediate yet
last @

\ else part of if/else/then construct
: m:else ( if_addr -- else_addr )
   m:here m:@                   \ d: if_addr else_addr
   0 2 m:litn, m:insn_jmp m:c,  \ d: if_addr else_addr
   swap                         \ d: else_addr if_addr
   m:then tail                  \ d: else_addr
   ; noexit imm

\ set the then word to immediate
F_IMM swap setflag

\ places the current address on the return stack
\ to later be used by words such as while, until, again, etc.
\ note the return stack currently has the caller address
\ on top, which is from the compile word
: m:begin ( r: rcomp -- r: label rcomp )
   r>                           \ d: rcomp | r:
   m:here m:@                   \ d: rcomp label | r:
   >r >r                        \ d: | r: label rcomp
   ; imm

\ iterates the loop if the current value on the
\ stack is false
: m:until ( r: label rcomp -- r: label rcomp )
   r> r@                        \ d: rcomp label | r: label
   m:insn_jz m:jump,            \ d: rcomp | r: label
   >r                           \ d: | r: label rcomp
   ; imm

\ iterates the loop regardless
: m:again  ( r: label rcomp -- r: label rcomp )
   r> r@                        \ d: rcomp label | r: label
   m:insn_jmp m:jump,           \ d: rcomp | r: label
   >r                           \ d: | r: label rcomp
   ; imm

\ iterates the loop with a call instead of a jump
: m:recurse ( r: label rcomp -- r: label rcomp )
   r> r@                        \ d: rcomp label | r: label
   m:insn_jsr m:jump,           \ d: rcomp | r: label
   >r                           \ d: | r: label rcomp
   ; imm

\ ends the begin block
: m:end ( r: label rcomp -- r: rcomp )
   r>                           \ d: rcomp | r: label
   r> drop                      \ d: rcomp | r:
   >r                           \ d: | r: rlabel
   ; imm

( implementation of the private / public framework )

\ start the scope
: m:scope{ ( -- )
   m:tmpbuf m:buf>start m:@     \ d: vtmpstart
   m:tmpbuf m:buf>here m:!      \ d:

   0 m:temp m:dict>next m:!
   0 m:temp m:dict>last m:!

   m:tmpbuf m:temp              \ d: tmpbuf temp
   2dup m:dict>code m:!         \ d: tmpbuf temp
   m:dict>data m:!              \ d:
   ;

\ stop the scope
: m:}scope ( -- )
   m:temp m:dict>next m:@       \ d: vnext
   m:current m:@                \ d: vnext vcurrent
   m:temp =                     \ d: vnext tempcurrent?
   if m:current m:! tail then
   m:context m:@                \ d: vnext vcontext
   m:temp =                     \ d: vnext tempcontext?
   if m:context m:! tail then
   drop
   ;

\ auxiliary declarations
: m:auxiliary ( -- )
   m:}scope                     \ d:
   m:current m:@                \ d: vcurrent
   m:temp m:dict>next m:!       \ d:
   m:temp m:current m:!         \ d:
   m:tmpbuf                     \ d: tmpbuf
   m:temp m:dict>code m:!       \ d:
   ;

\ the private declarations of the scope
: m:private ( -- )
   m:}scope                     \ d:
   m:current m:@                \ d: vcurrent
   dup                          \ d: vcurrent vcurrent
   m:temp m:dict>next m:!       \ d: vcurrent
   m:temp m:current m:!         \ d: vcurrent
   m:dict>code m:@              \ d: currentbuf
   m:temp m:dict>code m:!       \ d:
   ;

\ the public declarations of the scope
: m:public ( -- )
   m:}scope                     \ d:
   m:context m:@                \ d: vcontext
   m:temp m:dict>next m:!       \ d:
   m:temp m:context m:!         \ d:
   m:current m:@                \ d: vcurrent
   m:dict>code m:@              \ d: currentbuf
   m:temp m:dict>code m:!       \ d:
   ;

}scope

\ obtains the execution token of the next word
: m:' ( -- xt )
   word 2dup 0 m:lookup         \ d: c-str n addr
   dup =0                       \ d: c-str n addr zero?
   if drop unknown tail then
   nip nip m:>xt tail           \ d: xt
   ; noexit

\ word to create a variable
: m:var ( size -- )
   m:here m:@                   \ d: size vhere
   swap m:allot                 \ d: vhere
   m:create                     \ d: vhere
   m:lit,                       \ d:
   m:wrapup
   m:inl
   ;

," exec"

\ creates a deferred word
: m:defer ( -- )
   m:align
   m:here m:@                   \ d: size vhere
   0 m:,                        \ d: vhere
   m:create                     \ d: vhere
   m:lit,                       \ d:
   D0 m:c,                      \ compile "@"
   [ swap lit, lit, ]           \ d: c-str n
   2dup 0 m:lookup              \ d: c-str n addr
   dup =0                       \ d: c-str n addr zero?
   if drop unknown tail then
   nip nip m:>xt                \ d: xt
   m:insn_jmp m:jump,           \ compile jump to exec
   m:wrapup
   ;

\ sets the xt of a deferred word
: m:is ( xt -- )
   word 0 m:lookup              \ d: xt addr
   4 - m:!
   ;

( code for the ROM )
here @ moffset !

\ initialize global variables
m:forth m:current m:!
m:compiler m:context m:!
00 m:this m:!
00 m:flags m:c!
00 m:state m:c!
0A m:base m:c!

\ initialize the forth dictionary
00 m:forth m:dict>last m:!
00 m:forth m:dict>next m:!
m:wordbuf m:forth m:dict>code m:!
m:wordbuf m:forth m:dict>data m:!

\ initialize the compiler dictionary
00 m:compiler m:dict>last m:!
m:forth m:compiler m:dict>next m:!
m:wordbuf m:compiler m:dict>code m:!
m:wordbuf m:compiler m:dict>data m:!

\ initialize the wordbuffer
\ use smaller size here
100 m:wordbuf m:buf>here m:!
100 m:wordbuf m:buf>start m:!
10000 m:wordbuf m:buf>end m:!
100 m:wordbuf m:buf>off m:!

\ initialize the temp dictionary
00 m:temp m:dict>last m:!
00 m:temp m:dict>next m:!
m:tmpbuf m:temp m:dict>code m:!
m:tmpbuf m:temp m:dict>data m:!

\ initialize the tmpbuffer
\ make it smaller here to fit the current
\ wordbuf
10000 m:tmpbuf m:buf>here m:!
10000 m:tmpbuf m:buf>start m:!
20000 m:tmpbuf m:buf>end m:!
10000 m:tmpbuf m:buf>off m:!

\ initialize the TIB
50000 m:tib m:buf>here m:!
50000 m:tib m:buf>start m:!
51000 m:tib m:buf>end m:!
50000 m:tib m:buf>off m:!

\ Initialize the extra buffer
51000 m:extrabuf m:buf>here m:!
51000 m:extrabuf m:buf>start m:!
100000 m:extrabuf m:buf>end m:!
51000 m:extrabuf m:buf>off m:!
