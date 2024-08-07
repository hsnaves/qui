\ meta-compiler used to build the bootstrapping code
hex

\ the meta compilation words go into the meta dictionary
dictionary meta

( saved words from the host environment )
: h:IMM     [ IMM ] lit ; inl
: h:>xt     [ ' >xt ] lit >r ;
: h:>flags  [ ' >flags ] lit >r ;
: h:lookup  [ ' lookup ] lit >r ;

scope{
brief
: META_BUFFER_SIZE 20000 ; inl

public
\ address of the meta-buffer
: meta-buffer ( -- addr )
  [ 0 META_BUFFER_SIZE - allot ] lit
  ; inl

\ exit the meta-compilation dictionary
: meta-scrap ( -- )
  meta scrap tail
  ; noexit

\ exit the meta-compilation dictionary and meta-interpreter
: meta-quit ( -- )
  r> drop \ drops from the meta-interpreter
  meta-scrap tail
  ; noexit

\ to compile strings on the host address
: %" ( -- c-str n )
  0 ", tail
  ; noexit

( conversion to meta and host words )
\ host address to meta address
: addr>meta ( addr -- addr' )  meta-buffer - ; inl

\ meta address to host address
: addr>host ( addr -- addr' )  meta-buffer + ; inl

private
\ checks for write outside the buffer
: check-write ( addr n -- )
  + META_BUFFER_SIZE swap u<
  =0 if exit then
  " write outside meta-buffer" 2 error tail
  ; noexit

public
\ set the meta as the current dictionary
meta current !

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

0 ", ? "
: unknown ( c-str n -- )
  swap addr>host swap
  [ swap ] lit lit 0 error 1 error tail
  ; noexit

: copy ( c-str n dst -- )
  addr>host rot
  addr>host rot rot
  copy tail
  ; noexit

: index ( c-str c -- idx )
  swap addr>host swap
  index tail
  ; noexit

: word ( -- c-str n )
  word
  swap addr>meta swap
  ;

: flush ( iend istart -- )
  swap addr>host swap addr>host
  flush tail
  ; noexit

