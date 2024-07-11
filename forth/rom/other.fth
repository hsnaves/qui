\ implementation of the dot "." and related words
hex

\ set the context
: set-context ( dictn ... dict2 dict1 n -- )
  0 context !
  begin
    dup if swap use 1- again then
  end
  drop
  ;

\ postpone the execution of an immediate word
: postpone ( -- )
  ' I_JSR jump, tail
  ; noexit imm

\ obtains the first character of the next word
: char ( -- c )
  word drop c@
  ;

extra current !
\ halts the machine
: halt ( -- )
  100 status! tail
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

( *** implementation of the "words" word *** )
scope{
private
\ calls the xt on each word of the dictionary
\ and one with the zero address ( to signal the end of list )
: process-dict ( dict xt -- dict' xt )
  over >r >r dict>last @
  begin
    r@ exec
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
  2drop
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

scope{
private
\ auxiliary word to compare-word-and-advance
: compare-auxiliary ( best offset new-offset new-best -- best' offset'  )
  >r dup 0 >= if
    2dup >= if
      nip nip r@ swap 0
    then
  then
  drop rdrop
  ;

\ compares the word and the xt of the word with the current best
: compare-word-and-advance ( best offset addr word -- best' offset' addr word )
  dup =0 if exit then
  over >r >r r@ - r@ 1 shl
  compare-auxiliary
  r> r@ over >xt - swap dup >r 1 shl 1+
  compare-auxiliary
  r> r> swap
  ;

public
internal current !
\ resolve the location of the word nearest to a given address
\ the value of best is 2 times the address of the word
\ or 2 times the address of the word plus 1 (for the >xt part)
: resolve ( addr -- best offset )
  0 swap dup
  [ ' compare-word-and-advance ] lit
  (words) drop
  ;
forth current !

private
\ print the ">xt" suffix
" >xt"
: print-xt ( -- )
  [ swap ] lit lit type tail
  ; noexit

\ print the relative address
: print-relative-address ( best offset -- )
  swap dup 1 ushr >name type
  1 and if print-xt then
  [ char + ] lit emit . tail
  ;

public
\ prints the address relative to the nearest word (before address)
: a. ( addr -- )
  resolve
  over if print-relative-address tail then
  nip w. tail
  ; noexit
}scope

extra current !
scope{
private
\ temporary words ( to be changed later )
: dstack ( v -- addr ) 100 + sysreg tail ; noexit
: rstack ( v -- addr ) 200 + sysreg tail ; noexit

public
( *** implementation of the stack printing words *** )
\ prints the contents of the data stack
: ds. ( -- )
  dsp @
  dup =0 if 100 + then
  1- 0
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
      a. space 1+
      again
    then
  end
  2drop
  nl tail
  ; noexit
}scope
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
