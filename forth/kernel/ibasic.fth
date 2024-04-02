\ basic internal words
hex

( *** definition words for working with dictionaries *** )
\ node sibling
: node>next ( addr -- addr' )     ; inl
\ the value of the node
: node>val ( addr -- addr' ) 04 + ; inl

\ last word of dictionary
: dict>last ( addr -- addr ) 04 + ; inl
\ buffer for code in dictinary
: dict>code ( addr -- addr ) 08 + ; inl
\ buffer for data in dictionary
: dict>data ( addr -- addr ) 0C + ; inl

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

scope{
private
\ shows the buffer overflow error
," buffer overflow"
: overflow ( -- )
  [ swap ] lit lit
  error tail
  ; noexit

public
\ detects if the buffer will overflow after allocating n bytes
: %overflow? ( n buf -- )
  dup buf>end @
  swap buf>here @
  - swap
  u< if overflow tail then
  ;
}scope

\ allocates a given number of bytes in the buffer
: %allot ( n buf -- )
  2dup %overflow?
  buf>here swap
  over @
  + swap !
  ;

\ compile a value in a user-selected buffer
: %, ( v buf -- )
  4 over %overflow?
  buf>here tuck
  @ tuck !
  4 + swap !
  ;

\ compile a byte in a user-selected buffer
: %c, ( b buf -- )
  1 over %overflow?
  buf>here tuck
  @ tuck c!
  1+ swap !
  ;

\ compiles a string in a user-selected buffer
: %str, ( c-str n buf -- )
  dup buf>here @
  >r over swap %allot r>
  str-copy tail
  ; noexit

\ aligns the here pointer to multiple of a cell
: %align ( buf -- )
  dup buf>here @
  dup 3 + -4 and
  swap -
  swap %allot tail
  ; noexit
