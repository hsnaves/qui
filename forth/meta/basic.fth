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
ephemeral
: META_BUFFER_SIZE 20000 ; inl

public
\ address of the meta-buffer
: meta-buffer ( -- addr )
  [ META_BUFFER_SIZE alloc ] lit
  ; inl

\ exit the meta-compilation dictionary
: meta-exit ( -- )
  meta discard
  rdrop \ drops from the meta interpreter
  ;

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
," write outside meta-buffer"
: INVALIDWRITE_STR ( -- c-str n )
  [ swap ] lit lit
  ; inl

\ checks for write outside the buffer
: check-write ( addr n -- )
  + META_BUFFER_SIZE swap u<
  if INVALIDWRITE_STR error tail then
  ;

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
: type ( -- c-str n )
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

: compare ( c-str1 n1 c-str2 n2 -- neq? )
  >r >r >r
  addr>host
  r> r>
  addr>host
  r>
  compare tail
  ; noexit

: number ( c-str n -- num rem )
  swap addr>host swap
  number tail
  ; noexit

