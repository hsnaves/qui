\ implementation of the dot "."  and related words

hex

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
   dup 0A u<                    \ d: dig less10?
   if                           \ d: dig
      [ char 0 ] lit +          \ d: c
      exit
   then
   [ char A 0A - ] lit +        \ d: c
   ;

\ prints a digit to the output ( no space at end )
: d. ( dig --)
   d>c emit tail                \ d:
   ; noexit

public

\ prints a byte to the output in hexadecimal (no space)
: b. ( b -- )
   dup 04 ushr                  \ d: b {b>>4}
   0F and d.                    \ d: b
   0F and d. tail               \ d:
   ; noexit

\ prints a half-word to the output in hexadecimal (no space)
: h. ( h -- )
   dup 08 ushr                  \ d: h {h>>8}
   b. b. tail                   \ d:
   ; noexit

\ prints a word to the output in hexadecimal (no space)
: w. ( w -- )
   dup 10 ushr                  \ d: w {w>>16}
   h. h. tail                   \ d:
   ; noexit

\ prints an unsigned word to the output in the current
\ base (no space at end)
: u. ( u -- )
   begin                        \ d: u
      base c@ u/mod             \ d: rem quot
      dup =0                    \ d: rem quot zero?
      if drop d. tail then
      recurse                   \ d: rem
   end
   d. tail                      \ d:
   ; noexit

\ prints a signed integer to the output in the current
\ base (no space at end)
: . ( num -- )
   dup                          \ d: num num
   0 <                          \ d: num neg?
   if
      [ char - ] lit emit       \ print minus sign
      0 swap -                  \ d: -num
   then
   u. tail                      \ d:
   ; noexit

}scope

( *** implementation of the stack printing words *** )

\ prints the contents of the data stack
: ds. ( -- )
   dsp @ 1- 0                   \ d: num idx
   begin
      2dup u>                   \ d: num idx rem?
      if
         dup dstack @           \ d: num idx val
         . space                \ d: num idx
         1+                     \ d: num idx'
         again
      then
   end
   2drop                        \ d:
   dup . nl tail                \ print accumulator
   ; noexit

\ prints the contents of the return stack
: rs. ( -- )
   rsp @ 0                      \ d: num idx
   begin
      2dup u>                   \ d: num idx rem?
      if
         dup rstack @           \ d: num idx val
         . space                \ d: num idx
         1+                     \ d: num idx'
         again
      then
   end
   2drop                        \ d:
   nl tail
   ; noexit

( *** implementation of the dump word *** )

scope{
private

\ dumps a single row of hexadecimal numbers
: dump_hex ( addr n  -- addr' )
   over                         \ d: addr n addr
   [ wordbuf buf>off ] lit      \ d: addr n addr off
   @ -                          \ d: addr n addr'
   w. 2 spaces                  \ d: addr n
   begin
      dup if                    \ d: addr n
        over c@ b. space        \ d: addr n
        1 /str                  \ d: addr' n'
        again
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
      dup if                    \ d: addr n
        over c@                 \ d: addr n c
        dup                     \ d: addr n c c
        20 7E within =0         \ d: addr n c notprint?
        if
           drop [ char . ] lit  \ d: addr n c'
        then
        emit
        1 /str                  \ d: addr' n'
        again
      then
   end
   [ char | ] lit emit
   2drop
   ;

\ dumps a single row
: dump_row ( addr n -- addr' n' )
    dup                         \ d: addr n m
    10 over u<                  \ d: addr n m large?
    if drop 10 then             \ d: addr n m'
    >r                          \ d: addr n | r: m
    over r@ dump_hex            \ d: addr n | r: m
    10 r@ - dump_spaces         \ d: addr n | r: m
    over r@ dump_print          \ d: addr n | r: m
    r@ - swap r> + swap         \ d: addr' n'
    nl tail
   ; noexit

public

\ dumps the contents of memory at given address
: dump ( addr n -- )
   begin                        \ d: addr n
      dup if
         dump_row               \ d: addr' n'
         again
      then
   end
   2drop
   ;

}scope


( *** implementation of the words word *** )


scope{
private

\ prints the words in a given dictionary
: words1 ( dict -- )
   dict>last @                  \ d: addr
   begin
      dup if                    \ d: addr
         dup >name              \ d: addr c-str n
         type space             \ d: addr
         >link                  \ d: addr'
         again
      then
   end
   drop                         \ d:
   ;

public

\ prints the words in the context
: words ( -- )
   context @                    \ d: dict
   begin
      dup if                    \ d: dict
         dup words1 nl nl       \ d: dict
         node>next @            \ d: dict'
         again
      then
   end
   drop                         \ d:
   ;

}scope
