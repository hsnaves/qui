\ meta interpreter

hex

2 janum c! \ to compile large if

\ interprets a single word ( given as counted string )
: interpret ( c-str n -- )
   2dup state c@ lookup         \ d: c-str n addr
   dup if                       \ d: c-str n addr
      dup >xt                   \ d: c-str n addr xt
      swap >flags               \ d: c-str n xt flags
      dup F_IMM and             \ d: c-str n xt flags imm?
      state c@ =0 or            \ d: c-str n xt flags imm?'
      =0 if
         F_INL and              \ d: c-str n xt inline?
         if                     \ d: c-str n xt
            inline,             \ d: c-str n
         else
            I_JSR jump,         \ d: c-str n
         then                   \ d: c-str n
         2drop                  \ d:
         exit
      then
      drop                      \ d: c-str n xt
   then
   drop                         \ d: c-str n

   2dup                         \ d: c-str n c-str n
   swap addr>host swap          \ d: c-str n c-str' n
   state c@ h:lookup            \ d: c-str n addr
   dup if
      dup h:>xt                 \ d: c-str n addr xt
      swap h:>flags             \ d: c-str n xt flags
      h:F_IMM and               \ d: c-str n xt imm?
      state c@ =0 or            \ d: c-str n xt imm?'
      if
         nip nip                \ d: xt
         exec tail
      then
   then
   drop

   2dup number                  \ d: c-str n num rem
   if
      drop unknown tail
   then
   nip nip                      \ d: num
   state c@
   if lit, then
   ;

1 janum c!

\ meta Forth interpreter
\ it interprets all the words in an infinite loop
\ except when the :state turns to zero
: interpreter ( -- )
   0 state c!                   \ set the state to interpret
   begin                        \
      word                      \ d: c-str n
      interpret                 \ d: ...
      again
   end
   ;
