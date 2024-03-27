\ implementation of the scope variables

hex

\ initialize the tempbuf and temp
70 buf>start @ 70 buf>here !
00 60 dict>last !
70 60 dict>code !
70 60 dict>data !
00 80 ! \ set zero to templink
80 currnext ! \ set currnext to templink
current @ 84 ! \ set tempcurr to the current value
60 current ! \ set temp to current


\ set the current to temp
60 use

( define the location of temporary buffer and temporary dictionary )
: temp ( -- addr )     60 ; inl
: tmpbuf ( -- addr )   70 ; inl
: templink ( -- addr ) 80 ; inl
: tempcurr ( -- addr ) 84 ; inl

: tempnext    [ temp node>next   ] lit ; inl
: templast    [ temp dict>last   ] lit ; inl
: tempcode    [ temp dict>code   ] lit ; inl
: tempdata    [ temp dict>data   ] lit ; inl
: tmpbufhere  [ tmpbuf buf>here  ] lit ; inl
: tmpbufstart [ tmpbuf buf>start ] lit ; inl
: tmpbufoff   [ tmpbuf buf>off   ] lit ; inl
: tmpbufend   [ tmpbuf buf>end   ] lit ; inl

\ obtains the xt of the exec word
: exec-xt ( -- xt)
   [ ' exec ] lit
   ; inl

\ go to public declarations
tempcurr @ current !
temp tempcurr !
