\ implementation of the scope variables
hex

\ initialize the tempbuf and temp
7C buf>start @ 7C buf>here !
00 68 dict>last !
7C 68 dict>code !
7C 68 dict>data !
0 68 dict>index !
00 8C ! \ set zero to tnode
8C currnext ! \ set currnext to tnode
current @ 90 ! \ set tcurr to the current value
68 current ! \ set temp to current

\ set the current to temp
68 use

( define the location of temporary buffer and temporary dictionary )
: temp ( -- addr )     68 ; inl
: tmpbuf ( -- addr )   7C ; inl
: tnode ( -- addr )    8C ; inl
: tcurr ( -- addr )    90 ; inl

\ obtains the xt of the exec word
: exec-xt ( -- xt)
  [ ' exec ] lit
  ; inl

\ go to public declarations
tcurr @ current !
temp tcurr !
