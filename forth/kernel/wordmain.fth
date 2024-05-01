\ handling words in dictionary
hex

( *** implementation of the compare word *** )
\ compare two counted strings
\ returns true when not equal
\ assumes n1 is positive
: compare ( c-str1 n1 c-str2 n2 -- neq? )
  swap >r over =
  begin \ d: c-str1 n1 cont? | r: c-str2
    if
      over c@ r@ c@ =
      if 1/str r> 1+ >r dup again then
    then
  end
  nip rdrop
  ;

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
word meta 0 lookup nip nip =0
dup internal and swap =0 current @ and or

current @ over current !
align defer (i-lookup)
align defer (i-insert)
current !

scope{
private
\ finds a word in the dictionary
: lookup1 ( c-str n dict -- c-str n addr )
  dup dict>index @
  if
    >r r@ (i-lookup)
    dup if rdrop exit then
    drop r>
  then

  dict>last @
  begin
    dup if
      >r 2dup r@ >name compare
      if r> >link again then
      r>
    then
  end
  ;

\ finds a word in the current (and currnext) dictionaries
: lookupcurrent ( c-str n -- c-str n addr )
  current @ lookup1
  dup if exit then
  drop currnext @
  dup if lookup1 tail then
  ;

\ finds a word in the context
: lookupcontext ( c-str n -- c-str n addr )
  context @
  begin
    dup if
      >r r@ lookup1             \ d: c-str n addr | r: dict
      dup if rdrop exit then
      drop r> node>next @
      again
    then
  end
  ;

public
\ finds a word in current or context
\ if current? is true search current, otherwise search context
: lookup ( c-str n current? -- c-str n addr )
  if lookupcurrent tail then
  lookupcontext tail
  ; noexit

}scope

current @ swap current !
\ finds the next word from TIB in the context
: (find) ( current? -- addr )
  word rot lookup
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
  exit, this @ dup last !
  current @ dup dict>index @
  if 2dup (i-insert) then
  2drop 0 this !
  ;
