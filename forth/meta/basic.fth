\ meta-compiler used to build the kernel

hex

\ the meta compilation words go into the meta dictionary
dictionary meta

( saved words from the host environment )
: h:F_IMM   [ F_IMM ] lit ; inl
: h:>xt     [ ' >xt ] lit exec tail ; noexit
: h:>flags  [ ' >flags ] lit exec tail ; noexit
: h:lookup  [ ' lookup ] lit exec tail ; noexit

scope{
auxiliary
: META_BUFFER_SIZE 20000 ; inl
," memory exhausted" embed-str MEMORYERROR_STR
: allocate-buffer ( -- addr )
   META_BUFFER_SIZE alloc
   dup =0 if
      MEMORYERROR_STR error tail
   then
   ;

public

\ address of the meta-buffer
: meta-buffer ( -- addr )
   [ allocate-buffer ] lit
   ; inl

\ exit the meta-compilation dictionary
: meta-exit ( -- )
   abandon-last tail
   ; noexit

( conversion to meta and host words )
\ host address to meta address
: addr>meta ( addr -- addr' )
   meta-buffer -
   ; inl

\ meta address to host address
: addr>host ( addr -- addr' )
   meta-buffer +
   ; inl

META_BUFFER_SIZE \ pass the value

}scope

\ set the meta as the current dictionary
meta current !

scope{
auxiliary
: META_BUFFER_SIZE lit ; inl

private

," invalid write outside meta-buffer" embed-str INVALIDWRITE_STR

\ checks for write outside the buffer
: check-write ( addr n -- )
   +                            \ d: addr'
   META_BUFFER_SIZE swap u<     \ d: outside?
   if
      INVALIDWRITE_STR
      error tail
   then
   ;

public

( read / write in meta system )
\ read cell in meta system
: @ ( addr -- v )
   addr>host @
   ;

\ write cell in meta system
: ! ( v addr -- )
   dup 4 check-write
   addr>host !
   ;

\ read byte in meta system
: c@ ( addr -- v )
   addr>host c@
   ;

\ write byte in meta system
: c! ( v addr -- )
   dup 1 check-write
   addr>host c!
   ;

}scope

\ types a word in the meta address space
: type ( -- c-str n )
   swap addr>host swap
   type tail
   ; noexit

\ implementation of unknown in meta address space
: unknown ( c-str n -- )
   swap addr>host swap
   unknown tail
   ; noexit

\ implementation of str-copy in meta address space
: str-copy ( c-str n dst -- )
   addr>host rot
   addr>host rrot
   str-copy tail
   ; noexit

\ implementation of char-find in meta address space
: char-find ( c-str c -- idx )
   swap addr>host swap
   char-find tail
   ; noexit

\ obtains a word in the meta address space
: word ( -- c-str n )
   word
   swap addr>meta swap
   ;

\ compare two counted strings
\ returns true when not equal
: compare ( c-str1 n1 c-str2 n2 -- neq? )
   >r >r >r
   addr>host
   r> r>
   addr>host
   r>
   compare tail
   ; noexit

\ obtains a word in the meta address space
: number ( c-str n -- num rem )
   swap addr>host swap
   number tail
   ; noexit

