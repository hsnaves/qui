\ module for including other modules

base c@ \ save the base for later
hex

scope{
public

\ variable space
align here @ 14 allot

auxiliary

: module-current [ dup ] lit            ; inl
: global-buffer [ 4 + ] lit             ; inl

\ offsets within the structure
: file-name   ( addr -- addr' )     4 + ; inl
: file-offset ( addr -- addr' )     8 + ; inl
: prev-getc ( addr -- addr' )      0C + ; inl
: str-buffer ( addr -- addr' )     10 + ; inl
: buffer-start  ( addr -- addr' )  20 + ; inl

: str-buffer-here ( addr -- addr' )
  [ 0 str-buffer buf>here ] lit
  +
  ; inl

: str-buffer-off ( addr -- addr' )
  [ 0 str-buffer buf>off ] lit
  +
  ; inl

\ sizes of the buffer and of the structure
: buffer-size                       100 ; inl
: struct-size
   [ buffer-size buffer-start ] lit
   ; inl

\ size of the global buffer
: global-buffer-size               1000 ; inl

0 module-current ! \ set it to zero

private

\ initializes the global-buffer
: module_initialize ( -- )
   [ onboot @ ] lit exec
   global-buffer-size allocate
   dup [ global-buffer buf>here ] lit !
   dup [ global-buffer buf>start ] lit !
   0 [ global-buffer buf>off ] lit !
   global-buffer-size +
   [ global-buffer buf>end ] lit !
   ;
last @ >xt onboot !

\ allocate space in the global-buffer
: allocate-space ( size -- addr )
   3 + -3 and                   \ round the size to multiple of 4
   global-buffer buf>here @     \ d: size addr
   swap                         \ d: addr size
   global-buffer %allot tail
   ; noexit

\ install the module
: uninstall-module ( addr -- )
   dup prev-getc @              \ d: addr vgetc

   \ obtain the previous value of getc
   [ defer-ptr getc ] lit       \ d: addr vgetc getc
   !                            \ d: addr

   \ link the module
   dup node>next @              \ d: addr vnext
   module-current !             \ d: addr
   global-buffer buf>here !     \ d:
   ;

\ reads the file and fills the buffer
\ returns the number of bytes red
: fill-buffer ( addr -- n )
   dup file-name @ file-name!   \ d: addr
   dup file-offset @
   file-offset!                 \ d: addr
   dup buffer-start >r          \ d: addr | r: start
   r@ over                      \ d: addr start addr | r: start
   str-buffer-off !             \ d: addr | r: start
   r@ buffer-size
   file-buffer!                 \ d: addr | r: start
   file-read >r                 \ d: addr | r: start n
   dup file-offset              \ d: addr fileoffset | r: start n
   dup @ r@ + swap !            \ d: addr | start n
   r> r> over +                 \ d: addr n vhere
   rot                          \ d: n vhere addr
   str-buffer-here              \ d: n vhere here
   !                            \ d: n
   ;

\ implementation of getc for the module
: module-getc ( -- c )
   module-current @             \ d: addr
   dup str-buffer-off @         \ d: addr voff
   over str-buffer-here @       \ d: addr voff vhere
   over swap u<                 \ d: addr voff rem?
   =0 if
      drop                      \ d: addr
      dup fill-buffer           \ d: addr n
      =0 if
         uninstall-module       \ d: addr
         getc tail
      then
      dup str-buffer-off @      \ d: addr voff
   then

   dup c@                       \ d: addr voff c
   swap 1+                      \ d: addr c voff'
   rot str-buffer-off           \ d: c voff' off
   !                            \ d: c
   ;

\ install the current module
: install-module ( addr -- )
   \ obtain the previous value of getc
   [ defer-ptr getc ] lit       \ d: addr getc
   2dup @                       \ d: addr getc addr vgetc
   swap prev-getc !             \ d: addr getc
   [ ' module-getc ] lit        \ d: addr getc newgetc
   swap !                       \ d: addr

   \ link the module
   module-current               \ d: addr current
   2dup @                       \ d: addr current addr vcurrent
   swap node>next !             \ d: addr current
   !                            \ d:
   ;

\ initializes the structure for the module
\ except for the filename
: init-struct ( addr -- )
  \ initialize the structure
   dup str-buffer               \ d: addr buf
   dup buffer-start swap        \ d: addr start buf
   2dup buf>start !             \ d: addr start buf
   2dup buf>here !              \ d: addr start buf
   buf>off !                    \ d: addr
   \ buf>end was already initialized in allocate-space
   0 over file-offset !         \ d: addr
   install-module tail
   ; noexit

public

: include ( c-str -- okay? )
   dup 0 char-find              \ d: c-str n
   \ add 1 for the null character
   1+                           \ d: c-str n'
   dup struct-size +            \ d: c-str n size

   allocate-space               \ d: c-str n addr

   dup init-struct              \ d: c-str n addr
   dup struct-size +            \ d: c-str n addr dst
   swap over swap               \ d: c-str n dst dst addr
   file-name !                  \ d: c-str n dst
   str-copy 1                   \ d: 1
   ;

: include" ( -- )
   ," drop                      \ d: addr
   0 c,                         \ d: addr
   dup include                  \ d: addr okay?
   drop here !                  \ d:
   ;

}scope


