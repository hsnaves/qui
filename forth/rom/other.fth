\ implementation of the dot "."  and related words
hex

\ obtains the first character of the next word
: char ( -- c )
  word
  drop c@
  ;

extra current !

\ the random seed
align 4 var seed
1 seed !

\ generated a random number based on the seed
: rand ( -- v )
  seed @
  dup 1 and
  swap 1 ushr
  swap if AC000000 xor then
  dup seed !
  ;

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

scope{
private
\ digit to character word
: d>c ( dig -- c )
  dup 0A u<
  if [ char 0 ] lit + exit then
  [ char A 0A - ] lit +
  ;

\ prints a digit to the output ( no space at end )
: d. ( dig --)
  d>c emit tail
  ; noexit

public
\ prints a byte to the output in hexadecimal (no space)
: b. ( b -- )
  dup 04 ushr
  0F and d.
  0F and d. tail
  ; noexit

\ prints a half-word to the output in hexadecimal (no space)
: h. ( h -- )
  dup 08 ushr
  b. b. tail
  ; noexit

\ prints a word to the output in hexadecimal (no space)
: w. ( w -- )
  dup 10 ushr
  h. h. tail
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
}scope

extra current !

( *** implementation of the stack printing words *** )
\ prints the contents of the data stack
: ds. ( -- )
  dsp @ 1- 0
  begin
    2dup u>                     \ d: num idx rem?
    if
      dup dstack @
      . space 1+
      again
    then
  end
  2drop
  dup . nl tail                 \ print accumulator
  ; noexit

\ prints the contents of the return stack
: rs. ( -- )
  rsp @ 0
  begin
    2dup u>                     \ d: num idx rem?
    if
      dup rstack @
      . space 1+
      again
    then
  end
  2drop
  nl tail
  ; noexit

forth current !

( *** implementation of the dump word *** )

scope{
private
\ dumps a single row of hexadecimal numbers
: dump_hex ( addr n  -- addr' )
  over
  [ wordbuf buf>off ] lit @ -
  w. 2 spaces
  begin
    dup if
      over c@ b. space
      1/str again
    then
  end
  2drop
  ;

\ prints a sequence of 3*n+1 spaces
: dump_spaces ( n -- )
  3 * 1+ spaces tail
  ; noexit

\ dumps the printable characters
: dump_print ( addr n  -- addr' )
  [ char | ] lit emit
  begin
    dup if
      over c@
      dup 20 7E within =0       \ d: addr n c notprint?
      if drop [ char . ] lit then
      emit 1/str
      again
    then
  end
  [ char | ] lit emit
  2drop
  ;

\ dumps a single row
: dump_row ( addr n -- addr' n' )
  dup 10 over u<
  if drop 10 then
  >r
  over r@ dump_hex
  10 r@ - dump_spaces
  over r@ dump_print
  r@ - swap r> + swap
  nl tail
  ; noexit

public
extra current !
\ dumps the contents of memory at given address
: dump ( addr n -- )
  begin
    dup if dump_row again then
  end
  2drop
  ;
forth current !
}scope

( *** implementation of the words word *** )

scope{
private
\ prints the words in a given dictionary
: words1 ( dict -- )
  dict>last @
  begin
    dup if
      dup >name
      type space
      >link
      again
    then
  end
  drop
  ;

public
\ prints the words in the context
: words ( -- )
  context @
  begin
    dup if
      dup words1 nl nl
      node>next @
      again
    then
  end
  drop
  ;
}scope
