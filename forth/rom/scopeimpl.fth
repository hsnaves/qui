\ words for implementing the scope functionality

hex

\ creates a deferred word
: defer ( -- )
   here @                       \ d: addr
   0 ,                          \ d: addr
   create                       \ d: addr
   lit,                         \ d:
   D0 c,                        \ compile "@"
   exec-xt                      \ d: xt
   I_JMP jump,                  \ compile jump to exec
   wrapup                       \ terminates the word
   ;

\ finds the address of the pointer to the deferred word
: defer-ptr ( -- addr )
   find 4 -
   ;

\ sets the xt of a deferred word
: is ( xt -- )
   defer-ptr !
   ;

\ start the scope
: scope{ ( -- )
   \ set here to start
   tmpbufstart @ tmpbufhere !

   \ set zero to the last word of temp
   0 templast !

   \ clear the templink and tempcurr
   currnext @ templink !
   templink currnext !
   temp tempcurr !

   \ set temp code to tmpbuf
   tmpbuf tempcode !

   \ set temp data to tmpbuf
   tmpbuf tempdata !

   \ set the context to the temp
   temp use
   ;

\ the public declarations of the scope
: public ( -- )
   current @                    \ d: vcurrent
   temp =                       \ d: eq?
   if
      tempcurr @ current !
      temp tempcurr !
   then
   ;

\ the private declarations of the scope
: private ( -- )
   public                       \ d:
   current @                    \ d: vcurrent
   dup tempcurr !               \ d: vcurrent
   temp current !               \ d: vcurrent
   dict>code @                  \ d: currentbuf
   tempcode !                   \ d:
   ;

\ auxiliary declarations in the scope
: auxiliary ( -- )
   private
   tmpbuf tempcode !
   ;

\ stop the scope
: }scope ( -- )
   public
   tempnext @ context !
   0 tempnext !

\ set the currnext to the old value
   templink @ currnext !
   ;

auxiliary

\ size of the tib
: TMPBUF_SIZE 10000 ; inl

private

\ initializes the tmpbuf
: scope_initialize ( -- )
   [ onboot @ ] lit exec
   TMPBUF_SIZE alloc            \ d: addr
   dup tmpbufhere !             \ d: addr
   dup tmpbufstart !            \ d: addr
   dup tmpbufoff !              \ d: addr
   TMPBUF_SIZE +                \ d: vend
   tmpbufend !                  \ d:
   ;
last @ >xt onboot !

}scope
