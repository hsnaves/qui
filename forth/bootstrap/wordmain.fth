\ handling words in dictionary
hex

scope{
ephemeral
include" forth/bootstrap/flags.fth"

public
( *** implementation of the compare word *** )
\ compare two counted strings
\ returns true when not equal
\ assumes n1 is positive
: compare ( c-str1 n1 c-str2 n2 -- neq? )
  swap >r over =
  begin \ d: c-str1 n1 cont? | r: c-str2
    if
      over c@ 0 r@ c@ =
      if
        1/str r> 1 + >r dup again
      then
    then
  end
  nip r> drop
  ;

( *** words for accessing parts of a word in the dictionary *** )
\ obtains the flags for the word
: >flags ( addr -- fl )
  c@ [ F_EXT 1 - ] lit
  over u<
  [ 0 F_EXT - F_IMM or F_INL or ] lit
  or and
  ;

\ obtains the link to the next word
: >link ( addr -- addr' )
  dup 1 + over c@               \ d: addr addr' fl
  dup F_EXT and
  if
    F_LINK and
    if 1 + @ else 1 + [ swap ]  \ trick to save jumps
  else
    drop then c@ 8 signe
  then                          \ d: addr diff
  dup =0
  if nip exit then
  +
  ;

\ obtains the name of the word
: >name ( addr -- c-addr n )
  dup c@ swap 1 +               \ d: fl addr'
  over F_EXT and
  if
    dup 1 + swap c@
  else
    0 swap rot 1F and
  then                          \ d: fl addr len
  swap 1 + rot F_LINK and       \ d: len addr' link?
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

private
\ finds a word in the dictionary
: lookup1 ( c-str n dict -- c-str n addr )
  dict>last @
  begin
    dup if
      >r 2dup 0 r@ >name compare
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
      dup >r lookup1            \ d: c-str n addr | r: dict
      dup if r> drop exit then
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

( *** create and related words *** )

private
\ computes the link to the last word for a given word address
: link ( addr -- diff )
  last @ tuck swap -            \ d: vlast diff
  swap =0 =0 and
  ;

\ updates the flags according to the word length and
\ the current state of the buffers and the position of the
\ last word
\ returns the updated flags and the merged first byte
: updateflags ( fl n -- fl' fb )
  \ check for large word length
  >r 1F 0 r@ u<
  if F_EXT or then
  \ check if the code buffer is the same as the data buffer
  data buf>here >r
  here 0 r@ = =0                \ d: fl different? | r: n here
  if F_XT or then
  \ check for large link
  r> @ link
  dup 8 signe = =0              \ d: fl notshort? | r: n
  if F_LINK or then
  \ ensure that F_EXT is present when extra flags are set
  dup 1F and
  if F_EXT or then
  \ merge in the length when F_EXT is not set
  dup dup F_EXT and
  =0 if 0 r@ or then
  r> drop
  ;

public

\ creates a word in the current dictionary
: create ( -- )
  flags c@ word
  swap >r >r 0 r@
  updateflags                   \ d: fl' fb | r: c-str n
  data buf>here
  dup @ this !
  tuck %c,                      \ d: fl dhere | r: c-str n
  over F_EXT and
  if 0 r@ over %c, then
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
  this dup @ last !
  0 swap ! exit, tail
  ; noexit
