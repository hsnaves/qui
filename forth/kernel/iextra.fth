\ extra internal words
hex

\ compiles a string inplace and returns the
\ counted string on the stack
: ," ( -- c-str n )
  here @
  begin
    key dup [ char " ] lit =
    if
      drop here @
      over - exit
    then
    c,
    again
  end
  ;

