\ module for including other modules

hex

scope{
auxiliary

\ the maximum recursive depth of included files
: max-depth                           4 ; inl

\ offsets within the structure
: module-buffer ( addr -- addr' )       ; inl
: file-name   ( addr -- addr' )    10 + ; inl
: file-offset ( addr -- addr' )    14 + ; inl
: next-module ( addr -- addr' )    18 + ; inl
: prev-getc ( addr -- addr' )      1C + ; inl
: buffer-start  ( addr -- addr' )  20 + ; inl

\ sizes of the buffer and of the structure
: buffer-size                      1000 ; inl
: struct-size
   [ buffer-size buffer-start lit, ]
   ; inl

private

\ variable for the last struct created
align 4 var module-current

0 module-current ! \ set it to zero

\ get the depth of recursive inclusion
: get-depth ( -- n )
   0 module-current @           \ d: depth addr
   begin
      dup if
         swap 1+ swap           \ d: depth' addr
         next-module @          \ d: depth addr'
         again
      then
   end
   drop
   ;

\ computes the length of a zero terminated string
: cstr-length ( c-str -- n )
   0                            \ d: c-str n
   begin
      over c@                   \ d: c-str n c
      if
         1+ swap 1+ swap        \ d: c-str' n'
         again
      then
   end
   nip
   ;

\ performs the memory copy from source to destination
: memory-copy ( src n dst -- )
   >r                           \ d: src n | r: dst
   begin
      dup if
         over c@                \ d: str n c | r: dst
         r@ c!                  \ d: str n | r: dst
         r> 1+ >r               \ d: str n | r: dst'
         str1+                  \ d: str' n' | r: dst
         again
      then
   end
   r> 2drop drop                \ d:
   ;

\ allocate space at the end of wordbuf
: allocate-space ( size -- addr )
   >r                           \ d: | r: size
   \ first make sure that there is enough space at the
   \ end of wordbuf
   [ wordbuf buf>end lit, ] @   \ d: vend | r: size
   dup                          \ d: vend vend | r:size
   [ wordbuf buf>here lit, ] @  \ d: vend vend vhere | r: size
   -                            \ d: vend rem | r:size
   \ add 4 to struct-size for alignment
   r@ 4 + u<                    \ d: vend small? | r:size
   if r> 2drop 0 exit then

   \ compute the new end of wordbuf (align by 4)
   dup r> -                     \ d: vend addr
   -4 and                       \ d: vend addr'

   dup                          \ d: vend addr addr
   [ wordbuf buf>end lit, ] !   \ d: vend addr
   tuck module-buffer buf>end ! \ d: addr
   ;

\ install the module
: uninstall-module ( addr -- )
   dup prev-getc @              \ d: addr vgetc

   \ obtain the previous value of getc
   [
      word getc 0 lookup 4 - lit,
   ]                            \ d: addr vgetc getc
   !                            \ d: addr

   \ link the module
   next-module @                \ d: vnext
   module-current !             \ d:
   ;

\ reads the file and fills the buffer
\ returns the number of bytes red
: fill-buffer ( addr -- n )
   dup file-name @ file-setname \ d: addr
   dup file-offset @
   file-setoffset               \ d: addr
   dup buffer-start >r          \ d: addr | r: start
   r@ over                      \ d: addr start addr | r: start
   module-buffer buf>off !      \ d: addr | r: start
   r@ buffer-size
   file-setbuffer               \ d: addr | r: start
   file-read >r                 \ d: addr | r: start n
   dup file-offset              \ d: addr fileoffset | r: start n
   dup @ r@ + swap !            \ d: addr | start n
   r> r> over +                 \ d: addr n vhere
   rot module-buffer buf>here   \ d: n vhere here
   !                            \ d: n
   ;

\ implementation of getc for the module
: module-getc ( -- c )
   module-current @             \ d: addr
   dup module-buffer buf>off @  \ d: addr voff
   over module-buffer buf>here @ \ d: addr voff vhere
   over swap u<                 \ d: addr voff rem?
   =0 if
      drop                      \ d: addr
      dup fill-buffer           \ d: addr n
      =0 if
         uninstall-module       \ d:
         getc tail
      then
      dup module-buffer buf>off @ \ d: addr voff
   then

   dup c@                       \ d: addr voff c
   swap 1+                      \ d: addr c voff'
   rot module-buffer buf>off    \ d: c voff' off
   !
   ;

\ install the current module
: install-module ( addr -- )
   \ obtain the previous value of getc
   [
      word getc 0 lookup 4 - lit,
   ]                            \ d: addr getc
   2dup @                       \ d: addr getc addr vgetc
   swap prev-getc !             \ d: addr getc
   [
      word module-getc 0 lookup >xt lit,
   ]                            \ d: addr getc newgetc
   swap !                       \ d: addr

   \ link the module
   module-current               \ d: addr current
   2dup @                       \ d: addr current addr vcurrent
   swap next-module !           \ d: addr current
   !                            \ d:
   ;

\ initializes the structure for the module
\ except for the filename
: init-struct ( addr -- )
  \ initialize the structure
   dup module-buffer            \ d: addr buf
   dup buffer-start swap        \ d: addr start buf
   2dup buf>start !             \ d: addr start buf
   2dup buf>here !              \ d: addr start buf
   buf>off !                    \ d: addr
   \ buf>end was already initialized in allocate-space

   0 over file-offset !         \ d: addr

   install-module tail
   ; noexit

public

: module-include ( c-str -- okay? )
   \ check depth first
   max-depth get-depth u<       \ d: c-str deep?
   if drop 0 exit then

   \ add 1 for the null character
   dup cstr-length 1+           \ d: c-str n
   dup struct-size +            \ d: c-str n size

   allocate-space               \ d: c-str n addr
   dup =0 if nip nip exit then

   dup init-struct              \ d: c-str n addr
   dup struct-size +            \ d: c-str n addr dst
   swap over swap               \ d: c-str n dst dst addr
   file-name !                  \ d: c-str n dst
   memory-copy 1                \ d: 1
   ;


}scope

decimal
