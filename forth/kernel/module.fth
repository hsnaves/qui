\ module for including other modules
hex

scope{
public
\ variable space
align 8 buf>end allot

brief
: module-current [ dup ] lit            ; inl
: global-buffer [ 4 + ] lit             ; inl

\ offsets within the structure
: mod>filename   ( addr -- addr' )  4 + ; inl
: mod>offset ( addr -- addr' )      8 + ; inl
: mod>pgetc ( addr -- addr' )      0C + ; inl
: mod>strbuf ( addr -- addr' )     10 + ; inl
: mod>bufstart  ( addr -- addr' )  20 + ; inl

: mod>strbuf-here ( addr -- addr' )
  [ 0 mod>strbuf buf>here ] lit +
  ; inl

: mod>strbuf-off ( addr -- addr' )
  [ 0 mod>strbuf buf>off ] lit +
  ; inl

\ sizes of the buffer and of the structure
: buffer-size                       100 ; inl
: struct-size
  [ buffer-size mod>bufstart ] lit
  ; inl

\ size of the global buffer
: global-buffer-size               1000 ; inl

private

0 module-current ! \ set it to zero

\ initialize the global buffer
0 global-buffer buf>here !
0 global-buffer buf>start !
0 global-buffer buf>off !
0 global-buffer buf>end !

\ initializes the global-buffer
: module_initialize ( -- )
  [ onboot @ ] lit exec
  [ 0 global-buffer-size - ] lit allot
  dup [ global-buffer buf>here ] lit !
  dup [ global-buffer buf>start ] lit !
  global-buffer-size +
  [ global-buffer buf>end ] lit !
  ;
last @ >xt onboot !

\ allocate space in the global-buffer
: allocate-space ( size -- addr )
  3 + -4 and                    \ round the size to multiple of 4
  global-buffer %allot tail
  ; noexit

\ install the module
: uninstall-module ( addr -- )
  dup mod>pgetc @
  \ obtain the previous value of getc
  [ find getc >dptr ] lit !
  \ link the module
  dup node>next @ module-current !
  global-buffer buf>here !
  ;

" i/o error "
: f-error ( addr -- )
  [ swap ] lit lit 0 error
  dup uninstall-module
  mod>filename @ dup 0 index
  1 error tail
  ; noexit

\ reads the file and fills the buffer
\ returns the number of bytes red
: fill-buffer ( addr -- n )
  dup mod>filename @ f-name!
  dup mod>bufstart dup >r       \ d: addr start | r: start
  over mod>strbuf-off !
  dup mod>offset @
  0 r@ buffer-size 1 f-do       \ d: addr n | r: start
  dup 0 <
  if r> drop drop f-error tail then
  >r                            \ d: addr | r: start n
  dup mod>offset
  dup @ 0 r@ + swap !
  r> r> over +                  \ d: addr n vhere
  rot mod>strbuf-here !
  ;

\ implementation of getc for the module
: module-getc ( -- c )
  module-current @
  dup mod>strbuf-off @
  over mod>strbuf-here @
  over swap u< =0               \ d: addr voff full?
  if
    drop dup fill-buffer =0
    if uninstall-module 0A exit then
    dup mod>strbuf-off @
  then
  dup c@                        \ d: addr voff c
  swap 1 +
  rot mod>strbuf-off !
  ;

\ install the current module
: install-module ( addr -- )
  \ obtain the previous value of getc
  [ find getc >dptr ] lit
  2dup @
  swap mod>pgetc !
  [ ' module-getc ] lit
  swap !
  \ link the module
  module-current
  2dup @
  swap node>next !
  !
  ;

\ initializes the structure for the module
\ except for the filename
: init-struct ( addr -- )
  dup mod>strbuf
  over mod>bufstart swap
  2dup buf>start !
  2dup buf>here !
  buf>off !
  \ no need to initialize buf>end
  0 over mod>offset !
  install-module tail
  ; noexit

public

inner current !
\ include a module
: (include) ( c-str -- )
  dup 0 index
  \ add 1 for the null character
  1 + dup struct-size +
  allocate-space
  dup init-struct
  dup struct-size +
  swap over swap
  mod>filename !
  copy tail
  ; noexit
forth current !

\ include a module (inline)
: include" ( -- )
  ", 0 c, drop dup here !
  (include) tail
  ; noexit
}scope

