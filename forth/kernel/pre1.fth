\ header for including the inline words
hex

\ initialize the tempbuf and temp
current @ other !
60 current ! \ set temp to current

\ use the temp dictionary
context @ 60 ! 60 context !

( define the location of temporary buffer and temporary dictionary )
: temp ( -- addr )     60 ; inl
: tmpbuf ( -- addr )   70 ; inl

