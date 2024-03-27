\ last words of rom

hex

( *** implementation of stack words *** )

scope{
auxiliary

\ fix the implementation of the word
: fix_implementation ( -- )
   last @ >xt                   \ d: lastxt
   find tuck >name + !          \ d: addr
   F_IMM F_INL or swap          \ d: fl addr
   toggleflags tail             \ d:
   ; noexit

private

( *** return stack manipulation words *** )
: >r_impl ( n -- r: n )
   state c@
   if DD c, tail then
   r> swap >r >r
   ;
fix_implementation >r

: r>_impl ( r: n -- n )
   state c@
   if DE c, tail then
   r> r> swap >r
   ;
fix_implementation r>

: r@_impl ( r: n -- n | r: n )
   state c@
   if DF c, tail then
   r> r@ swap >r
   ;
fix_implementation r@

: rdrop_impl ( r: n -- n | r: n )
   state c@
   if DE c, D9 c, tail then
   r> rdrop >r
   ;
fix_implementation rdrop

}scope

scope{
private

( words for handling exceptions )

\ handles general exceptions
: general_exception ( status c-str n -- )
   1 channel c!                 \ d: status c-str n | r: pc
   type                         \ d: status | r: pc
   r>                           \ d: status pc
   dup w. space                 \ d: status pc
   c@ b. nl                     \ d: status
   terminate tail
   ; noexit

\ handles invalid instructions
," invalid instruction at "
: invalid_insn ( status -- )
   [ swap ] lit lit
   general_exception tail
   ; noexit

\ handles divide by zero errors
," divide by zero at "
: divide_by_zero ( status -- )
   [ swap ] lit lit
   general_exception tail
   ; noexit

," stack overflow at "
: stack_overflow ( status -- )
   [ swap ] lit lit
   general_exception tail
   ; noexit

\ current implementation of onexcept
: my_onexcept ( status -- )
   dup 1 = if
      invalid_insn tail
   then
   dup 2 = if
      divide_by_zero tail
   then
   dup 3 = if
      stack_overflow tail
   then
   terminate tail
   ; noexit
last @ >xt onexcept !

}scope

decimal

0 here @ file-buffer!
0 file-offset!
file-name" rom.bin"
file-write . nl
bye
