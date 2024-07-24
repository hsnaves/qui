\ here, last and other related words
hex

\ current position of the code buffer
: here ( -- addr )
  code buf>here tail
  ; noexit

\ position of last word in the current dictionary
: last ( -- addr )
  current @ dict>last
  ;

\ allocates a given number of bytes
: allot ( n -- addr )
  code %allot tail
  ; noexit

\ aligns the here pointer to multiple of a cell
: align ( -- )
  code %align tail
  ; noexit

\ compile a value in the code buffer
: , ( v -- )  here %, tail ; noexit

\ compile a byte in the code buffer
: c, ( b -- ) here %c, tail ; noexit

\ compiles a string in the code buffer
: str, ( c-str n -- )  here %str, tail ; noexit
