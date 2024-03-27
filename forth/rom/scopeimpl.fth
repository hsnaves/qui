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

\ start the scope
: scope{ ( -- )
   \ clear the tmpbuf
   tmpbufstart @ tmpbufhere !

   \ initialize the temp dictionary
   0 templast !
   tmpbuf tempcode !
   tmpbuf tempdata !

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
   abandon-last
   currnext node-unlink tail
   ; noexit

auxiliary

\ size of the tib
: TMPBUF_SIZE 10000 ; inl

private

\ initializes the tmpbuf
: scope_initialize ( -- )
   [ onboot @ ] lit exec
   TMPBUF_SIZE allocate         \ d: addr
   dup tmpbufhere !             \ d: addr
   dup tmpbufstart !            \ d: addr
   0 tmpbufoff !                \ d: addr
   TMPBUF_SIZE +                \ d: vend
   tmpbufend !                  \ d:
   ;
last @ >xt onboot !

}scope

