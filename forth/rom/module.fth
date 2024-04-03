\ module for including other modules
hex

scope{
public
\ variable space
align here @ 14 allot

ephemeral
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

0 module-current ! \ set it to zero

private

\ initialize the global buffer
0 global-buffer buf>here !
0 global-buffer buf>start !
0 global-buffer buf>off !
0 global-buffer buf>end !

\ initializes the global-buffer
: module_initialize ( -- )
  [ onboot @ ] lit exec
  global-buffer-size alloc
  dup [ global-buffer buf>here ] lit !
  dup [ global-buffer buf>start ] lit !
  global-buffer-size +
  [ global-buffer buf>end ] lit !
  ;
last @ >xt onboot !

\ allocate space in the global-buffer
: allocate-space ( size -- addr )
  3 + -4 and                    \ round the size to multiple of 4
  global-buffer buf>here @ swap
  global-buffer %allot tail
  ; noexit

\ install the module
: uninstall-module ( addr -- )
  dup mod>pgetc @
  \ obtain the previous value of getc
  [ defer-ptr getc ] lit !
  \ link the module
  dup node>next @
  module-current !
  global-buffer buf>here !
  ;

," could not read "
: file-error ( addr -- )
  1 channel c!
  [ swap ] lit lit type
  dup uninstall-module
  mod>filename @ dup 0 char-find
  error tail
  ; noexit

\ reads the file and fills the buffer
\ returns the number of bytes red
: fill-buffer ( addr -- n )
  dup mod>filename @ file-name!
  dup mod>bufstart >r           \ d: addr | r: start
  r@ over mod>strbuf-off !
  dup mod>offset @
  r@ buffer-size 1 file-do      \ d: addr n | r: start
  dup 0 <
  if rdrop drop file-error tail then
  >r                            \ d: addr | r: start n
  dup mod>offset
  dup @ r@ + swap !
  r> r> over +                  \ d: addr n vhere
  rot mod>strbuf-here !
  ;

\ implementation of getc for the module
: module-getc ( -- c )
  module-current @
  dup mod>strbuf-off @
  over mod>strbuf-here @
  over u<=                      \ d: addr voff full?
  if
    drop dup fill-buffer =0
    if uninstall-module getc tail then
    dup mod>strbuf-off @
  then
  dup c@                        \ d: addr voff c
  swap 1+
  rot mod>strbuf-off !
  ;

\ install the current module
: install-module ( addr -- )
  \ obtain the previous value of getc
  [ defer-ptr getc ] lit
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
  dup mod>bufstart swap
  2dup buf>start !
  2dup buf>here !
  buf>off !
  \ buf>end was already initialized in allocate-space
  0 over mod>offset !
  install-module tail
  ; noexit

public

internal current !
\ include a module
: (include) ( c-str -- )
  dup 0 char-find
  \ add 1 for the null character
  1+ dup struct-size +
  allocate-space
  dup init-struct
  dup struct-size +
  swap over swap
  mod>filename !
  str-copy
  ;
forth current !

\ include a module (inline)
: include" ( -- )
  ," 0 c, drop
  dup (include) here !          \ restore here pointer
  ;
}scope

