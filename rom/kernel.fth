
( *** reset trampoline *** )

\ placeholder for the jump
0 5 m:insn_jmp m:jumpn,

m:scope{

( *** forth dictionary *** )
m:public

( global variables )
m:: current ( -- addr )  00 m:; m:inl
m:: context ( -- addr )  04 m:; m:inl
m:: this ( -- addr )     08 m:; m:inl
m:: flags ( -- addr )    0C m:; m:inl
m:: state ( -- addr )    0D m:; m:inl
m:: base ( -- addr )     0E m:; m:inl
m:: forth ( -- addr )    10 m:; m:inl
m:: compiler ( -- addr ) 20 m:; m:inl
m:: wordbuf ( -- addr )  30 m:; m:inl
m:: extrabuf ( -- addr ) 40 m:; m:inl
m:: tib ( -- addr )      50 m:; m:inl

( basic words )
m:: =0    ( n -- {n==0} )          [ C4 m:c, ] m:; m:inl
m:: =     ( n1 n2 -- {n1==n2} )    [ C5 m:c, ] m:; m:inl
m:: u<    ( u1 u2 -- {u1<u2} )     [ C6 m:c, ] m:; m:inl
m:: <     ( n1 n2 -- {n1<n2} )     [ C7 m:c, ] m:; m:inl
m:: nop   ( -- )                   [ C8 m:c, ] m:; m:inl
m:: and   ( n1 n2 -- {n1&n2} )     [ C9 m:c, ] m:; m:inl
m:: or    ( n1 n2 -- {n1|n2} )     [ CA m:c, ] m:; m:inl
m:: xor   ( n1 n2 -- {n1^n2} )     [ CB m:c, ] m:; m:inl
m:: +     ( n1 n2 -- {n1+n2} )     [ CC m:c, ] m:; m:inl
m:: -     ( n1 n2 -- {n1-n2} )     [ CD m:c, ] m:; m:inl
m:: du*   ( u1 u2 -- lo high )     [ CE m:c, ] m:; m:inl
m:: u/mod ( u1 u2 -- rem quot )    [ CF m:c, ] m:; m:inl
m:: @     ( addr -- n )            [ D0 m:c, ] m:; m:inl
m:: !     ( n addr -- )            [ D1 m:c, ] m:; m:inl
m:: c@    ( addr -- c )            [ D2 m:c, ] m:; m:inl
m:: c!    ( c addr -- )            [ D3 m:c, ] m:; m:inl
m:: sge8  ( c -- n )               [ D4 m:c, ] m:; m:inl
m:: shl   ( n1 n2 -- {n1<<n2} )    [ D5 m:c, ] m:; m:inl
m:: ushr  ( u1 u2 -- {u1>>u2} )    [ D6 m:c, ] m:; m:inl
m:: shr   ( n1 n2 -- {n1>>n2} )    [ D7 m:c, ] m:; m:inl
m:: dup   ( n -- n n )             [ D8 m:c, ] m:; m:inl
m:: drop  ( n -- )                 [ D9 m:c, ] m:; m:inl
m:: swap  ( n1 n2 -- n2 n1 )       [ DA m:c, ] m:; m:inl
m:: over  ( n1 n2 -- n1 n2 n1 )    [ DB m:c, ] m:; m:inl
m:: rot   ( n1 n2 n3 -- n2 n3 n1 ) [ DC m:c, ] m:; m:inl

( some composite words )
m:: nip   ( n1 n2 -- n2 )           swap drop  m:; m:inl
m:: tuck  ( n1 n2 -- n2 n1 n2 )     swap over  m:; m:inl
m:: 2dup  ( n1 n2 -- n1 n2 n1 n2 )  over over  m:; m:inl
m:: 2drop ( n1 n2 -- )              drop drop  m:; m:inl
m:: rrot  ( n1 n2 n3 -- n3 n1 n2 )   rot  rot  m:; m:inl
m:: *     ( n1 n2 -- {n1*n2} )       du* drop  m:; m:inl
m:: 1+    ( n1 -- {n1+1} )  [  1 m:lit, ]   +  m:; m:inl
m:: 1-    ( n1 -- {n1+1} )  [ -1 m:lit, ]   +  m:; m:inl
m:: not   ( n1 -- {~n1} )   [ -1 m:lit, ] xor  m:; m:inl
m:: bool  ( n1 -- {n1!=0} )           =0   =0  m:; m:inl

\ obtains the current value of the pc
m:: pc ( -- pc )
   [ DF m:c, ]                  \ d: pc | r: pc
   m:;

\ executes a given function
m:: exec ( addr -- ... )
   dup =0                       \ d: addr zero?
   m:if drop [ m:exit, ] m:then
   pc                           \ d: addr pc
   4 + -                        \ d: rel_addr
   [ m:insn_jmp m:c, ]
   m:; m:noexit

\ word to be called when there is an error
m:defer onerror ( c-str n -- )

\ change to the compiler dictionary
m:compiler m:current m:!

\ last word of dictionary
m:: dict>last ( addr -- addr )
   m:; m:inl

\ dictionary sibling
m:: dict>next ( addr -- addr )
   04 +
   m:; m:inl

\ buffer for code in dictinary
m:: dict>code ( addr -- addr )
   08 +
   m:; m:inl

\ buffer for data in dictionary
m:: dict>data ( addr -- addr )
   0C +
   m:; m:inl

\ obtains the here pointer of the buffer
m:: buf>here ( addr -- addr )
   m:; m:inl

\ obtains the start pointer of the buffer
m:: buf>start ( addr -- addr )
   4 +
   m:; m:inl

\ obtains the end pointer of the buffer
m:: buf>end ( addr -- addr )
   8 +
   m:; m:inl

\ obtains the offset pointer of the buffer
m:: buf>off ( addr -- addr )
   0C +
   m:; m:inl

( useful words for compilation )
\ the code buffer
m:: code ( -- addr )
   current @
   dict>code @
   m:;

\ the data buffer
m:: data ( -- addr )
   current @
   dict>data @
   m:;

m:private

m:," buffer overflow"

m:: overflow ( -- )
   [ swap m:lit, m:lit, ]       \ d: c-str n
   onerror m:tail
   m:; m:noexit

m:public

\ detects if the buffer will overflow
m:: %overflow? ( n here -- )
   dup buf>end @                \ d: n here vend
   swap buf>start @             \ d: n vend vstart
   - swap                       \ d: rem n
   u<                           \ d: overflow?
   m:if overflow m:tail m:then
   m:;

\ compile a value in a user-selected buffer
m:: %, ( v here -- )
   4 over %overflow?            \ d: v here
   tuck                         \ d: here v here
   @ tuck !                     \ d: here vhere
   4 + swap !                   \ d:
   m:;

\ compile a byte in a user-selected buffer
m:: %c, ( b here -- )
   1 over %overflow?            \ d: v here
   tuck                         \ d: here b here
   @ tuck c!                    \ d: vhere
   1+ swap !                    \ d:
   m:;

\ advances the counted string by one character
m:: str1+ ( c-str n -- {c-str+1} {n-1} )
   1- swap 1+ swap              \ d: c-str' n'
   m:;

\ compiles a string in a user-selected buffer
m:: %str, ( c-str n here -- )
   [ m:>r, ]                    \ d: c-str n | r: here
   m:begin                      \ d: c-str n | r: here
      dup m:if                  \ d: c-str n | r: here
         over c@                \ d: c-str n c | r: here
         [ m:r@, ] %c,          \ d: c-str n | r: here
         str1+                  \ d: c-str' n' | r: here
         m:again
      m:then                    \ d: c-str 0 | r: here
   m:end
   2drop                        \ d: | r: here
   [ m:r>, ] drop               \ d:
   m:;

\ compile a value in the code buffer
m:: , ( v -- )
   code buf>here                \ d: v here
   %, m:tail
   m:; m:noexit

\ compile a byte in the code buffer
m:: c, ( b -- )
   code buf>here                \ d: b here
   %c, m:tail
   m:; m:noexit

\ compiles a string in the code buffer
m:: str, ( c-str n -- )
   code buf>here                 \ d: c-str n here
   %str, m:tail
   m:; m:noexit

\ compile a sequence of instructions inline
m:: inline, ( addr -- )
   m:begin
      dup c@                    \ d: addr c
      dup
      [ m:insn_ret m:lit, ] =   \ d: addr c ret?
      =0 m:if
         c, 1+ m:again
      m:then
   m:end
   2drop                        \ d:
   m:;

\ sign-extend n-bit value
m:: sge ( v bits -- v' )
   dup 20 u<                    \ d: v bits overflow?
   =0 m:if
      drop [ m:exit, ]
   m:then
   20 swap -                    \ d: v bits'
   [ m:>r, m:r@, ]              \ d: v bits | r: bits
   shl [ m:r>, ] shr            \ d: v
   m:;

\ compile a literal
m:: lit, ( v -- )
   m:begin
      dup dup                   \ d: v v v
      6 sge =                   \ d: v se6?
      m:if
         3F and 80 +            \ d: v'
         c, [ m:exit, ]         \ d:
      m:then
      dup 7 shr                 \ d: v {v>>7}
      m:recurse                 \ d: v
      7F and c, m:tail          \ d:
   m:end
   m:; m:noexit

\ compile a literal of a given fixed size
m:: litn, ( v n -- )
   m:begin
      1- dup                    \ d: v n' n'
      =0 m:if
         drop                   \ d: v
         3F and 80 +            \ d: v'
         c, m:tail
      m:then
      over 7 shr                \ d: v n {v>>7}
      swap m:recurse            \ d: v
      7F and c, m:tail          \ d:
   m:end
   m:; m:noexit

\ checks if a given value fits a literal of fixed size
m:: litn? ( v n -- b )
   7 * 1-                       \ d: v bits
   over swap                    \ d: v v bits
   sge =                        \ d: b
   m:;

\ compiles a jump to a target
m:: jump, ( target insn -- )
   [ m:>r, ]                    \ d: target | r: insn
   code buf>here @              \ d: target vhere | r: insn
   1+ -                         \ d: diff | r: insn
   1                            \ d: diff n | r: insn
   m:begin
      2dup -                    \ d: diff n v | r: insn
      over litn?                \ d: diff n ok? | r: insn
      =0 m:if
         1+ m:again
      m:then
   m:end
   swap over -                  \ d: n diff' | r: insn
   swap litn,                   \ d: | r: insn
   [ m:r>, ] c, m:tail          \ d:
   m:; m:noexit

\ compiles a jump of a given fixed size
m:: jumpn, ( target n insn -- )
   [ m:>r, m:>r, ]              \ d: target | r: insn n
   code buf>here @              \ d: target vhere | r: insn n
   [ m:r@, ] 1+ + -             \ d: v | r: insn n
   [ m:r>, ] litn,              \ d: | r: insn
   [ m:r>, ] c, m:tail          \ d:
   m:; m:noexit

\ compiles a return
m:: exit, ( -- )
   [ m:insn_ret m:lit, ]
   c, m:tail                    \ compile return
   m:; m:noexit

\ change to the forth dictionary
m:forth m:current m:!

( *** return stack manipulation words *** )
m:: >r ( n -- r: n )
   state c@
   m:if DD c, m:tail m:then
   [ m:r>, ] swap
   [ m:>r, m:>r, ]
   m:; m:imm

m:: r> ( r: n -- n )
   state c@
   m:if DE c, m:tail m:then
   [ m:r>, m:r>, ] swap
   [ m:>r, ]
   m:; m:imm

m:: r@ ( r: n -- n | r: n )
   state c@
   m:if DF c, m:tail m:then
   [ m:r>, m:r@, ] swap
   [ m:>r, ]
   m:; m:imm

m:private

\ Obtains the address to the data stack pointer
m:: dsp ( -- addr )
   -1 -8 !                      \ set the SCELL to -1
   -C                           \ return the address of DSTACK
   m:;

\ Obtains the address to the return stack pointer
m:: rsp ( -- addr )
   -1 -8 !                      \ set the SCELL to -1
   -10                          \ return the address of RSTACK
   m:;

m:public

\ obtain the value of the dsp
m:: dsp@ ( -- u )
   dsp @
   m:;

\ set the value of the dsp
m:: dsp! ( u -- )
   dsp !
   m:;

\ obtain the value of the rsp
m:: rsp@ ( -- u )
   rsp @
   m:;

\ set the value of the rsp
m:: rsp! ( u -- )
   [ m:r>, ]                    \ save the return address
   swap rsp !
   [ m:>r, ]                    \ restore it
   m:;

\ obtains the memory size
m:: stacksize ( -- u )
   -1C @
   m:;

\ obtains the memory size
m:: memsize ( -- u )
   -20 @
   m:;

( auxiliary words )

\ compiles an exit
m:: exit ( -- )
   state c@
   =0 m:if [ m:exit, ] m:then
   exit, m:tail
   m:; m:noexit m:imm

\ returns true if min <= v <= max
m:: within ( v min max -- t )
   rot tuck u<                  \ d: min v greater
   m:if                         \ check if (v > max)
       2drop 0 [ m:exit, ]      \ return false
   m:then
   swap u< =0                   \ return (v >= min)
   m:;

( I/O words )

\ terminate the program
m:: bye ( n -- )
   -18 !
   m:;

\ emits one character to the standard output
m:: emit ( c -- )
   -48 !
   m:;

\ address of the variable to use the standard error
m:: use_err ( -- addr )
   -4C
   m:; m:inl

\ obtains an input character from standard input
m:: getc ( -- c )
   -44 @
   m:;

\ prints a newline to the console
m:: nl ( -- )
   0A emit m:tail
   m:; m:noexit

\ prints space
m:: space
   20 emit m:tail
   m:; m:noexit

\ types a counted string to the standard output
m:: type ( c-str n -- )
   m:begin
      dup                       \ d: c-str n n
      =0 m:if                   \ d: c-str n
         2drop [ m:exit, ]      \ terminate
      m:then
      over c@                   \ d: c-str n c
      emit                      \ d: c-str n
      str1+                     \ d: c-str' n'
      m:again
   m:end
   m:; m:noexit

( *** input words *** )

m:auxiliary

\ checks for a newline
m:: nl? ( c -- b )
   0A =
   m:; m:inl

\ checks if a character is blank
m:: blank? ( c -- b )
   \ all characters before ! are blanks
   [ char ! m:lit, ] u<
   m:; m:inl

m:private

m:," TIB overflow"

\ prints an error messages that the TIB overflowed
m:: tiboverflow ( -- )
   [ swap m:lit, m:lit, ]       \ d: c-str n
   onerror m:tail
   m:; m:noexit

m:public

\ Obtains one line from the input
m:: line ( -- )
   [ m:tib m:buf>start m:lit, ] \ d: start
   @ dup                        \ d: vstart vstart
   [ m:tib m:buf>here m:lit, ] ! \ d: vstart
   [ m:tib m:buf>off m:lit, ] ! \ d:
   m:begin                      \ d:
      getc                      \ d: c
      [ m:tib m:buf>here m:lit, ] @ \ d: c vhere
      2dup c!                   \ d: c vhere
      1+                        \ d: c vhere'
      [ m:tib m:buf>here m:lit, ] ! \ d: c
      nl? =0                    \ d: notnl?
      m:if
         [ m:tib m:buf>here m:lit, ] @
         [ m:tib m:buf>end m:lit, ] @
         u<
         m:if m:again m:then
         tiboverflow
      m:then
   m:end
   m:;

( *** private words *** )
m:private

\ ensures that the TIB has some character
m:: ensuretib ( -- )
   m:begin
      [ m:tib m:buf>off m:lit, ] @ \ d: voffset
      [ m:tib m:buf>here m:lit, ] @ \ d: voffset vhere
      u< =0                     \ d: empty?
      m:if
         line
         m:again
      m:then
   m:end
   m:;

( *** public words *** )
m:public

\ Obtains a character from the TIB
m:: key ( -- c )
   ensuretib
   [ m:tib m:buf>off m:lit, ]   \ d: offset
   dup @                        \ d: offset voffset
   tuck 1+                      \ d: voffset offset voffset'
   swap !                       \ d: voffset
   c@                           \ d: c
   m:;

\ Obtains a word from the TIB
m:: word ( -- c-str n )
   m:begin
      key blank?                \ d: blank?
      m:if m:again m:then
   m:end
   [ m:tib m:buf>off m:lit, ]   \ d: offset
   @ dup 1- swap                \ d: c-str voffset
   [ m:tib m:buf>here m:lit, ] @ \ d: c-str voffset vhere
   m:begin
      2dup u<                   \ d: c-str voffset vhere rem?
      m:if                      \ d: c-str voffset vhere
         over c@ blank? =0      \ d: c-str voffset vhere nonblank?
         m:if
            swap 1+ swap m:again
         m:then
      m:then
   m:end                        \ d: c-str voffset vhere
   drop dup 1+                  \ d: c-str voffset voffset'
   [ m:tib m:buf>off m:lit, ] ! \ d: c-str voffset
   over -                       \ d: c-str n
   m:;

( *** private words *** )
m:private

\ character to digit word
m:: c>d ( c -- dig )
   dup                          \ d: c c
   [
      char 0 m:lit,
      char 9 m:lit,
   ]
   within                       \ d: c digit?
   m:if
      [ char 0 m:lit, ] -
      [ m:exit, ]               \ return c - '0'
   m:then
   dup                          \ d: c c
   [
      char A m:lit,
      char Z m:lit,
   ]
   within                       \ d: c letter?
   =0 m:if
      drop -1 [ m:exit, ]       \ return -1
   m:then
   [ char A 0A - m:lit, ] -     \ return c - 'A' + 10
   m:;

\ counted string to unsigned number
\ returns the parsed number together with the number of
\ remaining characters in the counted string
m:: str>unum ( c-str n -- u rem )
   swap 0                       \ d: n c-str 0
   m:begin                      \ d: n c-str u
      over c@                   \ d: n c-str u c
      c>d                       \ d: n c-str u dig
      base c@                   \ d: n c-str u dig vbase
      2dup u<                   \ d: n c-str u dig vbase okay
      m:if                      \ d: n c-str u dig vbase
         rot * +                \ d: n c-str u
         rot 1-                 \ d: c-str u n-1
         [ m:>r, m:r@, ] rot    \ d: u n c-str | r: n
         1+ rot [ m:r>, ]       \ d: n c-str' u n
         =0 m:until             \ d: n c-str u
         2dup                   \ d: n c-str u bogus bogus
      m:then                    \ d: n c-str u dig vbase
      2drop                     \ d: n c-str u
   m:end                        \ d: n c-str u
   nip swap                     \ d: u n
   m:;

( *** public words *** )
m:public

\ counted string to signed number
\ returns the parsed number and the number of remaining
\ characters in the counted string
m:: number ( c-str n -- num rem )
   1 over u<                    \ d: c-str n big_len?
   m:if                         \ d: c-str n
      over c@                   \ d: c-str n c
      [ char - m:lit, ]  =      \ d: c-str n is_eq?
      m:if                      \ d: c-str n
         str1+                  \ d: c-str' n'
         str>unum               \ d: num rem
         0 rot                  \ d: rem 0 num
         - swap                 \ d: -num rem
         [ m:exit, ]            \ return
      m:then
   m:then
   str>unum m:tail              \ tail call
   m:; m:noexit                 \ drop the final ret

\ compare two counted strings lexicographically
m:: compare ( c-str1 n1 c-str2 n2 -- cmp )
   rot                          \ d: c-str1 c-str2 n2 n1
   m:begin
      over =0 over =0 or        \ d: c-str1 c-str2 n2 n1 zero?
      m:if
         2dup swap -            \ d: c-str1 c-str2 n2 n1 cmp
      m:else
         1- [ m:>r, ]
         1- [ m:>r, ]           \ d: c-str1 c-str2 | r: n1' n2'
         over c@ over c@ -      \ d: c-str1 c-str2 cmp | r: n1 n2
         [ m:>r, ]              \ d: c-str1 c-str2 | r: n1 n2 cmp
         1+ swap 1+ swap        \ d: c-str1' c-str2' | r: n1 n2 cmp
         [ m:r>, m:r>, m:r>, ]  \ d: c-str1 c-str2 cmp n2 n1
         rot                    \ d: c-str1 c-str2 n2 n1 cmp
         dup =0                 \ d: c-str1 c-str2 n2 n1 cmp eq?
         m:if
            drop m:again        \ d: c-str1 c-str2 n2 n1
         m:then
      m:then
   m:end                        \ d: c-str1 c-str2 n2 n1 cmp
   [ m:>r, ]                    \ d: c-str1 c-str2 n2 n1 | r: cmp
   2drop 2drop                  \ d: | r: cmp
   [ m:r>, ]                    \ d: cmp
   m:;

( words for accessing parts of a word in the dictionary )

\ obtains the flags for the word
m:: >flags ( addr -- fl )
   c@                           \ d: fl
   dup [ m:F_EXT m:lit, ] and   \ d: fl ext?
   =0 m:if
      [
         m:F_EXT
         m:F_IMM or
         m:F_INL or
         m:lit,
      ]                         \ d: fl mask
      and                       \ d: fl
   m:then
   m:;

\ obtains the link to the next word
m:: >link ( addr -- addr' )
   dup >flags                   \ d: addr fl
   over 1+                      \ d: addr fl addr'

   over [ m:F_EXT m:lit, ] and  \ d: addr fl addr' ext?
   m:if 1+ m:then               \ d: addr fl addr'

   over [ m:F_XT m:lit, ] and   \ d: addr fl addr' xt?
   m:if 4 + m:then              \ d: addr fl addr'

   swap [ m:F_LINK m:lit, ] and \ d: addr addr' link?
   m:if
      @
   m:else
      c@ sge8
   m:then                       \ d: addr diff
   dup =0 m:if                  \ d: addr diff
      nip [ m:exit, ]
   m:then
   +                            \ d: addr'
   m:;

\ obtains the name of the word
m:: >name ( addr -- c-addr n )
   dup c@                       \ d: addr fl
   [ m:>r, ] 1+                 \ d: addr' | r: fl
   [ m:r@, m:F_EXT m:lit, ] and \ d: addr ext? | r: fl
   m:if
      dup 1+                    \ d: addr addr+1 | r: fl
      swap c@                   \ d: addr' len | r: fl
   m:else
      [ m:r@, ] 1F and          \ d: addr len | r: fl
      [ m:r>, ] E0 and
      [ m:>r, ]                 \ d: addr len | r: fl
   m:then
   swap                         \ d: len addr | r: fl
   [ m:r@, m:F_XT m:lit, ] and  \ d: len addr xt? | r: fl
   m:if 4 + m:then              \ d: len addr' | r: fl
   1+ [ m:r>, m:F_LINK m:lit, ] and \ d: len addr' link?
   m:if 3 + m:then              \ d: len addr'
   swap                         \ d: c-str len
   m:;

\ obtains the address of the first instruction
\ of the word
m:: >xt ( addr -- addr )
   dup >flags                   \ d: addr fl
   [ m:F_XT m:lit, ] and        \ d: addr xt?
   m:if 2 + @ [ m:exit, ] m:then
   >name +                      \ d: addr'
   m:;

\ position of last word in the current dictionary
m:: last ( -- addr )
   current @
   dict>last
   m:;

\ current position of the code buffer
m:: here ( -- addr )
   code buf>here
   m:;

m:private

\ finds a word in the dictionary at given address
m:: lookup1 ( c-str n dict -- addr )
   swap [ m:>r, ]
   swap [ m:>r, ]               \ d: dict | r: n c-str
   dict>last @                  \ d: addr | r: n c-str
   m:begin
       dup m:if                 \ d: addr | r: n c-str
          dup >name             \ d: addr c-str2 n2 | r: n c-str
          [ m:r>, m:r@, ]
          over [ m:>r, ]        \ d: addr c-str2 n2 c-str n | r: n c-str
          compare               \ d: addr cmp | r: n c-str
          m:if
              >link             \ d: addr' | r: n c-str
              m:again
          m:then
       m:then
       [ m:r>, m:r>, ] 2drop    \ d: addr
   m:end
   m:;

\ finds a word in the all the sibling dictionaries
m:: lookupn ( c-str n dict -- addr )
   m:begin
      dup m:if                  \ d: c-str n dict
         [ m:>r, ]              \ d: c-str n | r: dict
         2dup [ m:r@, ] lookup1 \ d: c-str n addr | r: dict
         dup m:if
            nip nip             \ d: addr | r: dict
            [ m:r>, ]
            drop [ m:exit, ]    \ d: addr
         m:then
         drop [ m:r>, ]         \ d: c-str n dict
         dict>next @            \ d: c-str n dict
         m:again
      m:then
   m:end
   nip nip                      \ d: 0
   m:;

m:public

\  finds a word in the context or in the current dictionary
m:: lookup ( c-str n skip? -- addr )
   [ m:>r, ]                    \ d: c-str n | r: skip?
   2dup                         \ d: c-str n c-str n | r: skip?
   context @ lookupn            \ d: c-str n addr | r: skip?
   [ m:r>, ]                    \ d: c-str n addr skip?
   over or                      \ d: c-str n addr skip?
   m:if
      nip nip [ m:exit, ]      \ d: addr
   m:then
   drop current @               \ d: c-str n dict
   lookup1 m:tail               \ d: addr
   m:; m:noexit

m:private

\ computes the link to the last word for a given word address
m:: link ( addr -- diff )
   last @                       \ d: addr vlast
   tuck swap - swap             \ d: diff vlast
   bool and                     \ d: diff'
   m:;

\ updates the flags according to the word length and
\ the current state of the buffers and the position of the
\ last word
\ returns the updated flags and the merged first byte
m:: updateflags ( fl n -- fl' fb )
   [ m:>r, ]                   \ d: fl | r: n

   \ check for large word length
   1F [ m:r@, ] u<              \ d: fl large? | r: n
   m:if
      [ m:F_EXT m:lit, ] or     \ d: fl' | r: n
   m:then

   \ check if the code buffer is the same as the data buffer
   data buf>here [ m:>r, ]      \ d: fl | r: n here
   code buf>here [ m:r@, ] =    \ d: fl same? | r: n here
   =0 m:if
      [ m:F_XT m:lit, ] or      \ d: fl' | r: n here
   m:then

   \ check for large link
   [ m:r>, ] @                  \ d: fl vhere | r: n
   link                         \ d: fl diff | r: n
   dup sge8 =                   \ d: fl short? | r: n
   =0 m:if
      [ m:F_LINK m:lit, ] or    \ d: fl' | r: n
   m:then

   \ ensure that F_EXT is present when extra flags are set
   dup 1F and                   \ d: fl extra? | r: n
   m:if
      [ m:F_EXT m:lit, ] or     \ d: fl' | r: n
   m:then

   \ merge in the length when F_EXT is not set
   dup dup
   [ m:F_EXT m:lit, ] and       \ d: fl fb ext? | r: n
   =0 m:if [ m:r@, ] or m:then  \ d: fl fb'  | r: n
   [ m:r>, ] drop               \ d: fl fb
   m:;

m:public

\ creates a word in the current meta dictionary
m:: create ( -- addr )
   flags c@                     \ d: fl
   word swap                    \ d: fl n c-str
   [ m:>r, m:>r, m:r@, ]        \ d: fl n | r: c-str n
   updateflags                  \ d: fl' fb | r: c-str n
   data buf>here                \ d: fl fb here | r: c-str n
   dup @ this !                 \ set the this pointer

   tuck %c,                     \ d: fl here | r: c-str n
   over [ m:F_EXT m:lit, ] and  \ d: fl here ext? | r: c-str n
   m:if
      [ m:r@, ] over %c,        \ d: fl here | r: c-str n
   m:then

   over [ m:F_XT m:lit, ] and   \ d: fl here xt? | r: c-str n
   m:if 0 over %, m:then        \ d: fl here | r: c-str n
                                \ the xt is to be populated later

   2dup                         \ d: fl here fl here | r: c-str n
   this @                       \ d: fl here fl here vthis | r: c-str n
   link                         \ d: fl here fl here diff | r: c-str n
   swap rot
   [ m:F_LINK m:lit, ] and      \ d: fl here diff here link? | r: c-str n
   m:if %, m:else %c, m:then    \ d: fl here | r: c-str n

   [ m:r>, m:r>, ] swap         \ d: fl here c-str n
   rot %str,                    \ d: fl
   [ m:F_XT m:lit, ] and        \ d: xt?
   m:if
      code buf>here @           \ d: vcodehere
      this @ 2 +                \ d: vcodehere xt-addr
      !                         \ d:
   m:then
   m:;

\ to end a word definition in the meta dictionary
m:: wrapup ( addr -- )
   exit,                        \ compile return
   this @ last !                \ set last to this
   0 this !                     \ set list to zero
   0 state c!                   \ set the state to 0
   m:;

m:," ? "

\ prints an error message of an unknown word
m:: unknown ( c-str n -- )
   [ swap m:lit, m:lit, ]       \ d: c-str n c-str' n'
   1 use_err !
   type
   0 use_err !
   onerror m:tail
   m:; m:noexit

\ interprets a single word ( given as counted string )
m:: interpret ( c-str n -- )
   2dup state c@ =0 lookup      \ d: c-str n addr
   dup m:if                     \ d: c-str n addr
      dup >xt                   \ d: c-str n addr xt
      swap >flags               \ d: c-str n xt flags
      dup
      [ m:F_IMM m:lit, ] and    \ d: c-str n xt flags imm?
      state c@ =0 or            \ d: c-str n xt flags imm?'
      m:if
         drop nip nip           \ d: xt
         exec m:tail
      m:then

      [ m:F_INL m:lit, ] and    \ d: c-str n xt inline?
      m:if                      \ d: c-str n xt
         inline,                \ d: c-str n
      m:else
         [ m:insn_jsr m:lit, ]  \ d: c-str n insn
         jump,                  \ d: c-str n
      m:then                    \ d: c-str n
      2drop                     \ d:
      [ m:exit, ]
   m:then

   drop                         \ d: c-str n
   2dup number                  \ d: c-str n num error?
   m:if
      drop unknown m:tail
   m:then
   nip nip                      \ d: num
   state c@
   m:if lit, m:then
   m:;

\ main Forth interpreter
\ it interprets all the words in an infinite loop
m:: interpreter ( -- )
   0 state c!                   \ set the state to interpret
   m:begin                      \ d:
      word                      \ d: c-str n
      interpret                 \ d: ...
      m:again
   m:end
   m:;

\ to end a word definition in the meta dictionary
m:: ; ( addr -- )
   wrapup m:tail
   m:; m:noexit m:imm

\ to start a word definition in the meta dictionary
m:: : ( -- addr )
   create                       \ call the word creation
   1 state c!                   \ set the state to 1
   m:;

\ changes the last opcode from a call to a jump
m:: tail ( -- )
   code buf>here @              \ d: vhere
   1- dup c@                    \ d: vhere' c
   [ m:insn_jsr m:lit, ] =      \ d: vhere' call?
   =0 m:if
      drop [ m:exit, ]
   m:then
   [ m:insn_jmp m:lit, ]
   swap c!                      \ d:
   m:; m:imm

\ first word for comments
m:: ( ( -- )
   m:begin
      key                       \ d: c
      [ char ) m:lit, ] =       \ d: end?
      m:until                   \ d:
   m:end
   m:; m:imm

\ line comments
m:: \ ( -- )
   line m:tail
   m:; m:noexit m:imm

( *** reset *** )

m:: quit ( -- )
   0 rsp!
   interpreter
   m:;

\ word to called to start the program
m:defer start ( -- )

m:' quit m:is start

\ word to be called when an exception is raised
m:defer onexception ( status -- )

m:private

\ default implementation of onerror
m:: default_onerror ( c-str n -- )
   1 use_err !
   type nl                      \ d:
   0 use_err !
   line                         \ flush the current line
   quit tail
   m:; noexit

m:' default_onerror m:is onerror

\ default implementation of onexception
m:: default_onexception ( status -- )
   bye m:tail
   m:; m:noexit

m:' default_onexception m:is onexception

m:public

\ performs the boot
m:: boot ( status -- )
   \ check if status is nonzero (for exceptions)
   dup m:if onexception m:tail m:then
   drop
   0 rsp!
   1 dsp!
   memsize
   extrabuf buf>end !
   start tail
   m:; noexit

m:}scope


\ set the right size of the word buffer
40000 m:wordbuf m:buf>end m:!

\ initialize the tmpbuffer
\ now set it to the proper size
40000 m:tmpbuf m:buf>here m:!
40000 m:tmpbuf m:buf>start m:!
50000 m:tmpbuf m:buf>end m:!
40000 m:tmpbuf m:buf>off m:!

( resolve the initial jump )
m:here m:@                      \ d: vhere
100 m:here m:!                  \ d: vhere
m:' boot
5 m:insn_jmp m:jumpn,           \ write the jump
m:here m:!

( dump rom file to standard output )
0 m:addr>host
m:here m:@ type

0 bye
