\ implementation of the scope variables
hex

\ initialize the tempbuf and temp
90 buf>start @ 90 buf>here !
00 7C dict>last !
90 7C dict>code !
90 7C dict>data !
0 7C dict>index !
current @ currnext !
7C current ! \ set temp to current

\ set the current to temp
7C use

( define the location of temporary buffer and temporary dictionary )
: temp ( -- addr )     7C ; inl
: tmpbuf ( -- addr )   90 ; inl

\ obtains the xt of the exec word
: exec-xt ( -- xt)
  [ ' exec ] lit
  ; inl

\ go to public declarations
currnext @ current !
temp currnext !

\ compile defer-ptr in internal dictionary
\ this is used by scopeimpl.fth
internal
