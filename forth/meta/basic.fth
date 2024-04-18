\ meta-compiler used to build the kernel
hex

\ the meta compilation words go into the meta dictionary
dictionary meta

( saved words from the host environment )
: h:F_IMM   [ F_IMM ] lit ; inl
: h:>xt     [ ' >xt ] lit >r ;
: h:>flags  [ ' >flags ] lit >r ;
: h:lookup  [ ' lookup ] lit >r ;

scope{
ephemeral
: META_BUFFER_SIZE 20000 ; inl

public
\ address of the meta-buffer
: meta-buffer ( -- addr )
  [ META_BUFFER_SIZE alloc ] lit
  ; inl

\ exit the meta-compilation dictionary
: meta-discard ( -- )
  meta discard tail
  ; noexit

\ exit the meta-compilation dictionary and meta-interpreter
: meta-exit ( -- )
  rdrop \ drops from the meta-interpreter
  meta-discard tail
  ; noexit

( conversion to meta and host words )
\ host address to meta address
: addr>meta ( addr -- addr' )  meta-buffer - ; inl

\ meta address to host address
: addr>host ( addr -- addr' )  meta-buffer + ; inl

META_BUFFER_SIZE \ pass the value
}scope

\ set the meta as the current dictionary
meta current !

scope{
ephemeral
: META_BUFFER_SIZE lit ; inl

private
\ checks for write outside the buffer
: check-write ( addr n -- )
  + META_BUFFER_SIZE swap u<
  =0 if exit then
  " write outside meta-buffer" fatal tail
  ; noexit

public
( read / write in meta system )
\ read cell in meta system
: @ ( addr -- v ) addr>host @ ; inl

\ write cell in meta system
: ! ( v addr -- )
  dup 4 check-write
  addr>host !
  ;

\ read byte in meta system
: c@ ( addr -- v ) addr>host c@ ; inl

\ write byte in meta system
: c! ( v addr -- )
  dup 1 check-write
  addr>host c!
  ;
}scope

\ re-implementation of words in the meta address space
: type ( c-str n -- )
  swap addr>host swap
  type tail
  ; noexit

: unknown ( c-str n -- )
  swap addr>host swap
  unknown tail
  ; noexit

: str-copy ( c-str n dst -- )
  addr>host rot
  addr>host rrot
  str-copy tail
  ; noexit

: char-find ( c-str c -- idx )
  swap addr>host swap
  char-find tail
  ; noexit

: word ( -- c-str n )
  word
  swap addr>meta swap
  ;
