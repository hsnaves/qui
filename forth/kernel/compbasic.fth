\ basic compiler words

hex

( *** definition words for working with dictionaries *** )

\ dictionary sibling
: dict>next ( addr -- addr )
   ; inl

\ last word of dictionary
: dict>last ( addr -- addr )
   04 +
   ; inl

\ buffer for code in dictinary
: dict>code ( addr -- addr )
   08 +
   ; inl

\ buffer for data in dictionary
: dict>data ( addr -- addr )
   0C +
   ; inl

( useful words for compilation )

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
: buf>here ( addr -- addr )
   ; inl

\ obtains the start pointer of the buffer
: buf>start ( addr -- addr )
   04 +
   ; inl

\ obtains the end pointer of the buffer
: buf>end ( addr -- addr )
   08 +
   ; inl

\ obtains the offset pointer of the buffer
: buf>off ( addr -- addr )
   0C +
   ; inl

scope{
private
," buffer overflow"             \ d: c-str n
: overflow ( -- )
   [ swap ] lit lit             \ compile the string
   error tail
   ; noexit

public

\ detects if the buffer will overflow after allocating
\ n bytes
: %overflow? ( n buf -- )
   dup buf>end @                \ d: n buf vend
   swap buf>start @             \ d: n vend vstart
   - swap                       \ d: rem n
   u< if overflow tail then     \ check for overflow
   ;

}scope


\ allocates a given number of bytes in the buffer
: %allot ( n buf -- )
   2dup %overflow?              \ d: n buf
   buf>here swap                \ d: here n
   over @                       \ d: here n vhere
   + swap !                     \ d:
   ;

\ compile a value in a user-selected buffer
: %, ( v buf -- )
   4 over %overflow?            \ d: v buf
   buf>here tuck                \ d: here v here
   @ tuck !                     \ d: here vhere
   4 + swap !                   \ d:
   ;

\ compile a byte in a user-selected buffer
: %c, ( b buf -- )
   1 over %overflow?            \ d: v buf
   buf>here tuck                \ d: here v here
   @ tuck c!                    \ d: here vhere
   1+ swap !                    \ d:
   ;

\ compiles a string in a user-selected buffer
: %str, ( c-str n buf -- )
   dup buf>here @               \ d: c-str n buf vhere
   >r                           \ d: c-str n buf | r: vhere
   over swap                    \ d: c-str n n buf | r: vhere
   %allot                       \ d: c-str n | r: vhere
   r>                           \ d: c-str n vhere
   str-copy tail                \ d:
   ; noexit

\ aligns the here pointer to multiple of a cell
: %align ( buf -- )
   dup buf>here @               \ d: buf vhere
   dup 3 +                      \ d: buf vhere vhere'
   -3 and                       \ d: buf vhere vhere'
   swap -                       \ d: buf n
   swap %allot tail             \ d:
   ; noexit
