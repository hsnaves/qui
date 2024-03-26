\ basic compiler words

hex

\ compiles a string inplace and returns the
\ counted string on the stack
: ," ( -- c-str n )
   here @                       \ d: c-str
   begin
      key                       \ d: c-str c
      dup [ char " ] lit =      \ d: c-str c end?
      if                        \ d: c-str c
         drop                   \ d: c-str
         here @                 \ d: c-str here
         over - exit            \ d: c-str n
      then
      c,                        \ d: c-str
      again
   end
   ;

