\ words for implementing the scope functionality

hex

\ start the scope
: scope{ ( -- )
   \ clear the tmpbuf
   [ tmpbuf buf>start ] lit @
   [ tmpbuf buf>here ] lit !

   \ initialize the temp dictionary
   0 [ temp dict>last ] lit !
   tmpbuf [ temp dict>code ] lit !
   tmpbuf [ temp dict>data ] lit !

   \ clear the templink and tempcurr
   currnext templink node-link
   temp tempcurr !

   \ set the context to the temp
   temp use tail
   ; noexit

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
   [ temp dict>code ] lit !     \ d:
   ;

\ auxiliary declarations in the scope
: auxiliary ( -- )
   private
   tmpbuf
   [ temp dict>code ] lit !
   ;

\ stop the scope
: }scope ( -- )
   public
   abandon-last
   currnext node-unlink tail
   ; noexit

\ creates a deferred word
: defer ( -- )
   here @                       \ d: addr
   0 ,                          \ d: addr
   create                       \ d: addr
   lit,                         \ d:
   D0 c,                        \ compile "@"
   exec-xt                      \ d: xt
   I_JMP jump,                  \ compile jump to exec
   wrapup tail                  \ terminates the word
   ; noexit

\ finds the address of the pointer to the deferred word
: defer-ptr ( -- addr )
   find 4 -
   ;

\ sets the xt of a deferred word
: is ( xt -- )
   defer-ptr !
   ;

auxiliary

\ size of the tib
: TMPBUF_SIZE 10000 ; inl

private

\ initializes the tmpbuf
: scope_initialize ( -- )
   [ onboot @ ] lit exec
   TMPBUF_SIZE allocate         \ d: addr
   dup
   [ tmpbuf buf>here ] lit !    \ d: addr
   dup
   [ tmpbuf buf>start ] lit !   \ d: addr
   0 [ tmpbuf buf>off ] lit !   \ d: addr
   TMPBUF_SIZE +                \ d: vend
   [ tmpbuf buf>end ] lit !     \ d:
   ;
last @ >xt onboot !

}scope

