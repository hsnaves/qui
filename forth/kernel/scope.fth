\ implementation of the scope variables
hex

\ initialize the tempbuf and temp
70 buf>start @ 70 buf>here !
00 60 dict>last !
70 60 dict>code !
70 60 dict>data !
current @ currnext !
60 current ! \ set temp to current

\ set the current to temp
60 use

( define the location of temporary buffer and temporary dictionary )
: temp ( -- addr )     60 ; inl
: tmpbuf ( -- addr )   70 ; inl

\ go to public declarations
currnext @ current !
temp currnext !

\ compile defer-ptr in internal dictionary
\ this is used by scopeimpl.fth
internal
