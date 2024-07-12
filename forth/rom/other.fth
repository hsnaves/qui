\ implementation of the other words
hex

\ word to embed a constant
: const ( v -- )
  create lit, wrapup inl tail
  ; noexit

\ word to create a dictionary
: dictionary ( -- )
  align [ 4 dict>data ] lit
  allot dup const
  0 over dict>last !
  0 over node>next !
  wordbuf over dict>code !
  wordbuf swap dict>data !
  ;

\ set the context
: set-context ( dictn ... dict2 dict1 n -- )
  0 context !
  begin
    dup if swap use 1 - again then
  end
  drop
  ;

\ obtains the first character of the next word
: char ( -- c )
  word drop c@
  ;

internal current !
\ digit to character word
: d>c ( dig -- c )
  dup 0A u<
  if [ char 0 ] lit + exit then
  [ char A 0A - ] lit +
  ;

\ prints a digit to the output (no space at the end)
: d. ( dig -- )
  d>c emit tail
  ; noexit

forth current !
\ prints an unsigned word to the output in the current
\ base (no space at end)
: u. ( u -- )
  begin
    base c@ u/mod
    dup =0
    if drop d. tail then
    recurse
  end
  d. tail
  ; noexit

\ prints a signed integer to the output in the current
\ base (no space at end)
: . ( num -- )
  dup 0 <
  if
    [ char - ] lit emit
    0 swap -
  then
  u. tail
  ; noexit

( *** implementation of the "words" word *** )
scope{
private
\ calls the xt on each word of the dictionary
\ and one with the zero address ( to signal the end of list )
: process-dict ( dict xt -- dict' xt )
  over >r >r dict>last @
  begin
    0 r@ exec
    dup if
      >link again
    then
  end
  drop r> r> node>next @ swap
  ;

public
internal current !
\ implementation of words with a callback
: (words) ( xt -- )
  context @ swap
  begin
    over if
      process-dict again
    then
  end
  drop drop
  ;
forth current !

private
\ print a word
: print-word ( word -- word )
  dup if
     dup >name type space tail
  then
  nl nl tail
  ; noexit

public
\ prints the words in the context
: words ( -- )
  [ ' print-word ] lit
  (words) tail
  ; noexit
}scope
