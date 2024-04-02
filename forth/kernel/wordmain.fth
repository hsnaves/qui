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

\ finds a word in the dictionary
: lookup1 ( c-str n dict -- addr )
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

\ finds a word in the current (and sibling) dictionaries
: lookupcurrent ( c-str n -- addr )
  currnext >r
  begin
    2dup r@ node>val @
    lookup1                     \ d: c-str n addr | r: chain
    dup if
      dup c@                    \ small optimization instead of >flags
      F_IMM and =0
      if
        r> swap >r 2drop drop
        r> exit
      then
    then
    drop                        \ d: c-str n | r: chain
    r> node>next @
    dup if >r again then
  end
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

\ finds the next word from TIB in the context
: find ( -- addr )
  word 2dup 0 lookup
  dup if nip nip exit then
  drop unknown tail
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
  this @ last !
  0 this !
  ;
