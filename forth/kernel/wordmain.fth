\ handling words in dictionary
hex

( *** words for accessing parts of a word in the dictionary *** )
\ obtains the flags for the word
: >flags ( addr -- fl )
  c@ [ F_EXT 1- ] lit
  over u<
  [ 0 F_EXT - F_IMM or F_INL or ] lit
  or and
  ;

\ obtains the link to the next word
: >link ( addr -- addr' )
  dup 1+ over c@                \ d: addr addr' fl
  dup F_EXT and
  if
    F_LINK and
    if 1+ @ else 1+ c@ 8 signe then
  else
    drop c@ 8 signe
  then                          \ d: addr diff
  dup =0
  if nip exit then
  +
  ;

\ obtains the name of the word
: >name ( addr -- c-addr n )
  dup c@ swap 1+                \ d: fl addr'
  over F_EXT and
  if
    dup 1+ swap c@
  else
    0 swap rot 1F and
  then                          \ d: fl addr len
  swap 1+ rot F_LINK and        \ d: len addr' link?
  if 3 + then
  swap
  ;

\ obtains the address of the first instruction of the word
: >xt ( addr -- addr )
  dup >name +
  swap >flags
  F_XT and if @ then
  ;

( *** implementation of the lookup word *** )

\ detect if the meta compiler is present
\ the stack will contain the address of the internal
\ dictionary or the meta dictionary
word meta 0 lookup =0
dup internal and swap =0 current @ and or

current @ over current !
align defer (i-lookup)
align defer (i-insert)
current !

\ finds a word in the dictionary
: lookup1 ( c-str n dict -- addr )
  dup dict>index @
  if
    >r 2dup r@ (i-lookup)
    dup if rdrop nip nip exit then
    drop r>
  then

  swap >r swap >r
  dict>last @
  begin                         \ d: addr | r: n c-str
    dup if
      dup >name
      r> r@ over >r
      compare
      if >link again then
    then
    rdrop rdrop
  end
  ;

scope{
private

\ finds a word in the dictionary that is not immediate
: lookup1-nonimm ( c-str n dict -- addr )
  lookup1
  dup if
    dup c@                      \ small optimization instead of >flags
    F_IMM and =0 and
  then
  ;

\ finds a word in the current (and currnext) dictionaries
: lookupcurrent ( c-str n -- addr )
  2dup current @ lookup1-nonimm
  dup if nip nip exit then
  drop currnext @
  dup if lookup1-nonimm tail then
  nip nip
  ;

\ finds a word in the context
: lookupcontext ( c-str n -- addr )
  context @
  begin
    dup if
      >r 2dup r@ lookup1        \ d: c-str n addr | r: dict
      dup if
        nip nip rdrop exit
      then
      drop r> node>next @
      again
    then
  end
  nip nip
  ;

public
\ finds a word in the context
\ if include? is true, it also searches in the current dictionary
\ and this search is done before searching the word in the context
\ if the word is in the current dictionary, it is only returned
\ if it is not immediate
: lookup ( c-str n include? -- addr )
  if
    2dup lookupcurrent
    dup if nip nip exit then
    drop
  then
  lookupcontext tail
  ; noexit

}scope

current @ swap current !
\ finds the next word from TIB in the context
: (find) ( include? -- addr )
  >r word 2dup r> lookup
  dup if nip nip exit then
  drop unknown tail
  ; noexit
current !

\ finds the next word from TIB in the context
: find ( -- addr )
  0 (find) tail
  ; noexit

( *** create and related words *** )

scope{
private
\ computes the link to the last word for a given word address
: link ( addr -- diff )
  last @ tuck swap -            \ d: vlast diff
  swap bool and
  ;

\ updates the flags according to the word length and
\ the current state of the buffers and the position of the
\ last word
\ returns the updated flags and the merged first byte
: updateflags ( fl n -- fl' fb )
  \ check for large word length
  >r 1F r@ u<
  if F_EXT or then
  \ check if the code buffer is the same as the data buffer
  data buf>here >r
  code buf>here r@ <>           \ d: fl different? | r: n here
  if F_XT or then
  \ check for large link
  r> @ link
  dup 8 signe <>                \ d: fl notshort? | r: n
  if F_LINK or then
  \ ensure that F_EXT is present when extra flags are set
  dup 1F and
  if F_EXT or then
  \ merge in the length when F_EXT is not set
  dup dup F_EXT and
  =0 if r@ or then
  rdrop
  ;

public
\ creates a word in the current dictionary
: create ( -- addr )
  flags c@ word swap >r >r r@
  updateflags                   \ d: fl' fb | r: c-str n
  data buf>here
  dup @ this !
  tuck %c,                      \ d: fl dhere | r: c-str n
  over F_EXT and
  if r@ over %c, then
  2dup
  this @ link
  swap rot F_LINK and
  if %, else %c, then           \ d: fl dhere | r: c-str n
  dup r> r> swap
  rot %str,                     \ d: fl dhere
  swap F_XT and
  if
    dup @ swap
    0 swap %,
    here @ swap !
    0                           \ unused
  then
  drop
  ;
}scope

\ to end a word definition in the dictionary
: wrapup ( addr -- )
  exit,
  this @ dup last !
  current @ dup dict>index @
  if 2dup (i-insert) then
  2drop
  0 this !
  ;
