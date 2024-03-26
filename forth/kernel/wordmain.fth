\ handling words in dictionary

hex

( *** words for accessing parts of a word in the dictionary *** )

\ obtains the flags for the word
: >flags ( addr -- fl )
   c@                           \ d: fl
   dup F_EXT and                \ d: fl ext?
   =0 if
      [
         0 F_EXT -
         F_IMM or
         F_INL or
      ]
      lit and                   \ d: fl'
   then
   ;

\ obtains the link to the next word
: >link ( addr -- addr' )
   dup >flags                   \ d: addr fl
   over 1+                      \ d: addr fl addr'

   over F_EXT and               \ d: addr fl addr' ext?
   if 1+ then                   \ d: addr fl addr'

   swap F_LINK and              \ d: addr addr' link?
   if @ else c@ 8 signe then    \ d: addr diff

   dup =0                       \ d: addr diff zero?
   if nip exit then
   +                            \ d: addr'
   ;

\ obtains the name of the word
: >name ( addr -- c-addr n )
   dup c@                       \ d: addr fl
   swap 1+                      \ d: fl addr'
   over F_EXT and               \ d: fl addr ext?
   if
      dup 1+                    \ d: fl addr addr+1
      swap c@                   \ d: fl addr len
   else
      0 swap rot                \ d: 0 addr fl
      1F and                    \ d: 0 addr len
   then
   swap                         \ d: fl len addr
   1+ rot F_LINK and            \ d: len addr' link?
   if 3 + then                  \ d: len addr'
   swap                         \ d: c-str len
   ;

\ obtains the address of the first instruction
\ of the word
: >xt ( addr -- addr )
   dup >name +                  \ d: addr addr'
   swap >flags                  \ d: addr fl
   F_XT and                     \ d: addr xt?
   if @ then                    \ d: addr'
   ;


( *** implementation of the lookup word *** )

scope{
private

\ finds a word in the dictionary at given address
: lookup1 ( c-str n dict -- addr )
   swap >r swap >r              \ d: dict | r: n c-str
   dict>last @                  \ d: addr | r: n c-str
   begin
       dup if                   \ d: addr | r: n c-str
          dup >name             \ d: addr c-str2 n2 | r: n c-str
          r> r@ over >r         \ d: addr c-str2 n2 c-str n | r: n c-str
          compare               \ d: addr cmp | r: n c-str
          if >link again then
       then
       rdrop rdrop              \ d: addr
   end
   ;

\ finds a word in the current (and sibling) dictionaries
: lookupcurrent ( c-str n -- addr )
   currnext >r                  \ d: c-str n | r: chain
   begin
      2dup                      \ d: c-str n c-str n | r: chain
      r@  4 + @                 \ d: c-str n c-str n dict | r: chain
      lookup1                   \ d: c-str n addr | r: chain
      dup if
         dup >flags             \ d: c-str n addr vflags | r: chain
         F_IMM and              \ d: c-str n addr imm? | r: chain
         =0 if
            r> swap             \ d: c-str n chain addr
            >r 2drop drop       \ d: | r: addr
            r> exit             \ d: addr
         then
      then
      drop                      \ d: c-str n | r: chain
      r> @                      \ d: c-str n chain'
      dup =0                    \ d: c-str n chain zero?
      if
         nip nip exit           \ return zero
      then
      >r                        \ d: c-str n | r: chain
      again
   end
   ; noexit

\ finds a word in the context
: lookupcontext ( c-str n -- addr )
   context @                    \ d: c-str n dict
   begin
      dup if                    \ d: c-str n dict
         >r                     \ d: c-str n | r: dict
         2dup r@ lookup1        \ d: c-str n addr | r: dict
         dup if
            nip nip             \ d: addr | r: dict
            rdrop exit          \ d: addr
         then
         drop r>                \ d: c-str n dict
         dict>next @            \ d: c-str n dict
         again
      then
   end
   nip nip                      \ d: 0
   ;

public


\ finds a word in the context
\ if include? is true, it also searches in the current dictionary
\ and this search is done before searching the word in the context
\ if the word is in the current dictionary, it is only returned
\ if it is not immediate
: lookup ( c-str n include? -- addr )
   if
      2dup lookupcurrent        \ d: c-str n addr
      dup if
         nip nip exit           \ d: addr
      then
      drop                      \ d: c-str n
   then
   lookupcontext tail           \ d: addr
   ; noexit

}scope

\ finds the next word from TIB in the context
: find ( -- addr )
   word 2dup 0 lookup           \ d: c-str n addr
   dup =0                       \ d: c-str n addr zero?
   if drop unknown tail then
   nip nip
   ;



( *** create and related words *** )

scope{
private

\ computes the link to the last word for a given word address
: link ( addr -- diff )
   last @                       \ d: addr vlast
   tuck swap - swap             \ d: diff vlast
   bool and                     \ d: diff'
   ;

\ updates the flags according to the word length and
\ the current state of the buffers and the position of the
\ last word
\ returns the updated flags and the merged first byte
: updateflags ( fl n -- fl' fb )
   >r                           \ d: fl | r: n

   \ check for large word length
   1F r@ u<                     \ d: fl large? | r: n
   if F_EXT or then             \ d: fl' | r: n

   \ check if the code buffer is the same as the data buffer
   data buf>here >r             \ d: fl | r: n here
   code buf>here r@ =           \ d: fl same? | r: n here
   =0 if F_XT or then           \ d: fl' | r: n here

   \ check for large link
   r> @                         \ d: fl vhere | r: n
   link                         \ d: fl diff | r: n
   dup 8 signe =                \ d: fl short? | r: n
   =0 if F_LINK or then         \ d: fl' | r: n

   \ ensure that F_EXT is present when extra flags are set
   dup 1F and                   \ d: fl extra? | r: n
   if F_EXT or then             \ d: fl' | r: n

   \ merge in the length when F_EXT is not set
   dup dup
   F_EXT and                    \ d: fl fb ext? | r: n
   =0 if r@ or then             \ d: fl fb'  | r: n
   rdrop                        \ d: fl fb
   ;

public

\ creates a word in the current dictionary
: create ( -- addr )
   flags c@                     \ d: fl
   word swap                    \ d: fl n c-str
   >r >r r@                     \ d: fl n | r: c-str n
   updateflags                  \ d: fl' fb | r: c-str n
   data buf>here                \ d: fl fb dhere | r: c-str n
   dup @ this !                 \ set the this pointer

   tuck %c,                     \ d: fl dhere | r: c-str n
   over F_EXT and               \ d: fl dhere ext? | r: c-str n
   if
      r@ over %c,               \ d: fl dhere | r: c-str n
   then

   2dup                         \ d: fl dhere fl dhere | r: c-str n
   this @                       \ d: fl dhere fl dhere vthis | r: c-str n
   link                         \ d: fl dhere fl dhere diff | r: c-str n
   swap rot F_LINK and          \ d: fl dhere diff dhere link? | r: c-str n
   if %, else %c, then          \ d: fl dhere | r: c-str n

   dup                          \ d: fl dhere dhere | r: c-str n
   r> r> swap                   \ d: fl dhere dhere c-str n
   rot %str,                    \ d: fl dhere

   swap F_XT and                \ d: dhere xt?
   if
      dup @ swap                \ d: vdhere dhere
      0 swap %,                 \ d: vdhere
      here @                    \ d: vdhere vhere
      swap !                    \ d:
      0                         \ d: unused
   then
   drop                         \ d:
   ;

}scope

\ to end a word definition in the dictionary
: wrapup ( addr -- )
   exit,                        \ compile return
   this @ last !                \ set last to this
   0 this !                     \ set list to zero
   ;
