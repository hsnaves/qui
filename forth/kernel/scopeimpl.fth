\ words for implementing the scope functionality
hex

\ start the scope
: scope{ ( -- )
  [ tmpbuf buf>end ] lit @

  \ clear the tmpbuf
  [ tmpbuf buf>start ] lit @ dup
  [ tmpbuf buf>here ] lit !

  flush \ flush the cache

  \ initialize the temp dictionary
  0 [ temp dict>last ] lit !
  tmpbuf [ temp dict>code ] lit !
  tmpbuf [ temp dict>data ] lit !

  \ copy the offset of the code buffer into tmpbuf
  code buf>off @ [ tmpbuf buf>off ] lit !

  temp currnext !
  temp use tail
  ; noexit

\ the public declarations of the scope
: public ( -- )
  current @ temp =
  if
    currnext @ current !
    temp currnext !
  then
  ;

\ the private declarations of the scope
: private ( -- )
  public
  current @ dup currnext !
  temp current !
  dict>code @
  [ temp dict>code ] lit !
  ;

\ ephemeral declarations in the scope
: ephemeral ( -- )
  private
  tmpbuf [ temp dict>code ] lit !
  ;

\ stop the scope
: }scope ( -- )
  public
  0 currnext !
  temp discard tail
  ; noexit

\ creates a deferred word
: defer ( -- )
  here @ 0 ,
  create l,
  [ ' @ c@ ] lit c,
  [ ' exec c@ ] lit c,
  wrapup tail
  ; noexit

\ swaps to the dictionary in the stack
current @ swap current !
\ finds the address of the pointer to the deferred word
: defer-ptr ( addr -- addr' ) 4 - ; inl
current ! \ restore current

\ sets the xt of a deferred word
: is ( xt -- )
  find defer-ptr !
  ;

ephemeral
: TMPBUF_SIZE 10000 ; inl

private
\ initializes the tmpbuf
: scope_initialize ( -- )
  [ onboot @ ] lit exec
  [ 0 TMPBUF_SIZE - ] lit allot
  dup [ tmpbuf buf>here ] lit !
  dup [ tmpbuf buf>start ] lit !
  TMPBUF_SIZE + [ tmpbuf buf>end ] lit !
  ;
last @ >xt onboot !
}scope

