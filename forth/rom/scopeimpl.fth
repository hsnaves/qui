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

  \ clear the tnode and tcurr
  currnext tnode node-link
  temp tcurr !

  \ set the context to the temp
  temp use tail
  ; noexit

\ the public declarations of the scope
: public ( -- )
  current @ temp =
  if
     tcurr @ current !
     temp tcurr !
  then
  ;

\ the private declarations of the scope
: private ( -- )
  public
  current @
  dup tcurr !
  temp current !
  dict>code @
  [ temp dict>code ] lit !
  ;

\ auxiliary declarations in the scope
: auxiliary ( -- )
  private
  tmpbuf [ temp dict>code ] lit !
  ;

\ stop the scope
: }scope ( -- )
  public
  discard*
  currnext node-drop tail
  ; noexit

\ creates a deferred word
: defer ( -- )
  here @
  0 ,
  create
  lit,
  D0 c,                        \ compile "@"
  exec-xt
  I_JMP jump,
  wrapup tail
  ; noexit

\ swaps to the dictionary in the stack
current @ swap current !

\ finds the address of the pointer to the deferred word
: defer-ptr ( -- addr )
  find 4 -
  ;

current ! \ restore current

\ sets the xt of a deferred word
: is ( xt -- )
  defer-ptr !
  ;

auxiliary
: TMPBUF_SIZE 10000 ; inl

private
\ initializes the tmpbuf
: scope_initialize ( -- )
   [ onboot @ ] lit exec
   TMPBUF_SIZE alloc
   dup [ tmpbuf buf>here ] lit !
   dup [ tmpbuf buf>start ] lit !
   0 [ tmpbuf buf>off ] lit !
   TMPBUF_SIZE + [ tmpbuf buf>end ] lit !
   ;
last @ >xt onboot !
}scope

