\ header for including the inline words
hex

\ initialize the tempbuf and temp
current @ other !
60 current ! \ set temp to current

\ use the temp dictionary
context @ 60 ! 60 context !

\ word for handling dictionaries
: dict>next ( addr -- addr )       ; inl
: dict>last ( addr -- addr )  04 + ; inl
: dict>code ( addr -- addr )  08 + ; inl
: dict>data ( addr -- addr )  0C + ; inl

\ words for handling buffers
: buf>here ( addr -- addr )        ; inl
: buf>start ( addr -- addr )  04 + ; inl
: buf>end ( addr -- addr )    08 + ; inl
: buf>off ( addr -- addr )    0C + ; inl

\ temporary buffer and temporary dictionary
: temp ( -- addr )     60 ; inl
: tmpbuf ( -- addr )   70 ; inl

\ go to public declarations
other @ current !
temp other !

