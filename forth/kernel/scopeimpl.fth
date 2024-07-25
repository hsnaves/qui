\ words for implementing the scope functionality
hex

\ the current stack should be
\ d: inner

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

  temp other !
  temp use tail
  ; noexit

\ the public declarations of the scope
: public ( -- )
  current @ temp =
  if
    other @ current !
    temp other !
  then
  ;

\ the private declarations of the scope
: private ( -- )
  public
  current @ dup other !
  temp current !
  dict>code @
  [ temp dict>code ] lit !
  ;

\ brief declarations in the scope
: brief ( -- )
  private
  tmpbuf [ temp dict>code ] lit !
  ;

\ stop the scope
: }scope ( -- )
  public
  0 other !
  temp scrap tail
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
\ gets the pointer to the deferred word
: >dptr ( addr -- addr' ) 4 - ; inl
current ! \ restore current

\ sets the xt of a deferred word
: is ( xt -- )
  find >dptr !
  ;

brief
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

