\ basic internal words
hex

( *** definition words for working with dictionaries *** )
\ node sibling
: node>next ( addr -- addr' )      ; inl

\ last word of dictionary
: dict>last ( addr -- addr )  04 + ; inl
\ buffer for code in dictinary
: dict>code ( addr -- addr )  08 + ; inl
\ buffer for data in dictionary
: dict>data ( addr -- addr )  0C + ; inl

( *** useful words for compilation *** )
\ the current code buffer
: code ( -- addr )
  current @
  dict>code @
  ;

\ the current data buffer
: data ( -- addr )
  current @
  dict>data @
  ;

( *** words for compiling using buffers *** )
\ obtains the here pointer of the buffer
: buf>here ( addr -- addr )       ; inl
\ obtains the start pointer of the buffer
: buf>start ( addr -- addr ) 04 + ; inl
\ obtains the end pointer of the buffer
: buf>end ( addr -- addr )   08 + ; inl
\ obtains the offset pointer of the buffer
: buf>off ( addr -- addr )   0C + ; inl

\ detects if the buffer will overflow after allocating n bytes
" buffer overflow"
: %free? ( n buf -- )
  dup buf>end @
  swap buf>here @ -
  swap u<
  if [ rot rot swap ] lit lit 2 error tail then
  ;

\ allocates a given number of bytes in the buffer
: %allot ( n buf -- addr )
  2dup %free?
  buf>here dup @
  rot over + rot !
  ;

\ compile a value in a user-selected buffer
: %, ( v buf -- )
  4 over %free?
  buf>here tuck
  @ tuck !
  4 + swap !
  ;

\ compile a byte in a user-selected buffer
: %c, ( b buf -- )
  1 over %free?
  buf>here tuck
  @ tuck c!
  1 + swap !
  ;

\ compiles a string in a user-selected buffer
: %str, ( c-str n buf -- )
  over swap %allot
  str-copy tail
  ; noexit

\ aligns the here pointer to multiple of a cell
: %align ( buf -- )
  dup buf>here @
  dup 3 + -4 and
  swap -
  swap %allot drop
  ;
