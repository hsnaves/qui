\ implementation of TIB related words
hex

scope{
brief
include" forth/bootstrap/inline.fth"

1000
: TIB_SIZE    [ dup ] lit ; inl
: ALLOT_SIZE  [ 0 swap - ] lit ; inl

private
\ initializes the tib
: tib_initialize ( -- )
  [ onboot @ ] lit exec
  ALLOT_SIZE allot
  dup [ tib buf>here ] lit !
  dup [ tib buf>start ] lit !
  dup [ tib buf>off ] lit !
  TIB_SIZE + [ tib buf>end ] lit !
  ;
last @ >xt onboot !

brief
: eof? ( c -- b ) 0 < ; inl
: nl? ( c -- b ) 0A = ; inl

private
" TIB overflow"
: tiboverflow ( -- )
  [ swap ] lit lit 1 error tail
  ; noexit

public
\ Obtains one line from the input
: line ( -- )
  [ tib buf>end ] lit @ >r
  [ tib buf>start ] lit @
  dup [ tib buf>here ] lit !
  dup [ tib buf>off ] lit !
  begin
    getc swap 2dup c! 1 +
    over eof? if bye tail then
    swap nl? =0
    if
      dup 0 r@ u< =0 until
      tiboverflow tail
    then
  end
  [ tib buf>here ] lit !
  r> drop
  ;

private
\ ensures that the TIB has some character
: ensuretib ( -- )
  begin
    [ tib buf>off ] lit @
    [ tib buf>here ] lit @
    u< =0 if line again then
  end
  ;

public
\ Obtains a character from the TIB
: key ( -- c )
  ensuretib
  [ tib buf>off ] lit
  dup @ tuck 1 + swap ! c@
  ;

brief
: blank? ( c -- b )
  [ key ! ] lit u<
  ; inl

public
\ Obtains a word from the TIB
: word ( -- c-str n )
  begin
    key blank? =0 until
  end
  [ tib buf>off ] lit @
  dup 1 - swap
  [ tib buf>here ] lit @ >r
  begin
    dup 0 r@ u<
    if
      dup c@ blank? =0
      if 1 + again then
    then
  end
  r> drop dup 1 +
  [ tib buf>off ] lit !
  over -
  ;
}scope
