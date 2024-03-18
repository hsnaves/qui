\ here, last and other related words

hex

\ current position of the code buffer
: here ( -- addr )
   code buf>here
   ;

\ position of last word in the current dictionary
: last ( -- addr )
   current @ dict>last
   ;

\ allocates a given number of bytes
: allot ( n -- )
   code %allot tail
   ; noexit

\ aligns the here pointer to multiple of a cell
: align ( -- )
   code %align tail
   ; noexit
