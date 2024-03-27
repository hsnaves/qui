\ interpreter word code

hex

2 janum c! \ to compile large if

\ interprets a single word ( given as counted string )
: interpret ( c-str n -- )
   2dup state c@ lookup         \ d: c-str n addr
   dup if                       \ d: c-str n addr
      dup >xt                   \ d: c-str n addr xt
      swap >flags               \ d: c-str n xt flags
      dup
      F_IMM and                 \ d: c-str n xt flags imm?
      state c@ =0 or            \ d: c-str n xt flags imm?'
      if
         drop nip nip           \ d: xt
         exec tail
      then

      F_INL and                 \ d: c-str n xt inline?
      if                        \ d: c-str n xt
         inline,                \ d: c-str n
      else
         I_JSR jump,            \ d: c-str n
      then
      2drop                     \ d:
      exit
   then

   drop                         \ d: c-str n
   2dup number                  \ d: c-str n num rem
   if
      drop unknown tail
   then
   nip nip                      \ d: num
   state c@
   if lit, then
   ;

1 janum c!

\ main Forth interpreter
\ it interprets all the words in an infinite loop
: interpreter ( -- )
   0 state c!                   \ set the state to interpret
   begin                        \ d:
      word                      \ d: c-str n
      interpret                 \ d: ...
      again
   end
   ;
