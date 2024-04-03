\ implementation of hash tables for dictionaries
hex

scope{
public
\ variable space
align here @ 14 allot

ephemeral
: table-buffer [ dup ] lit           ; inl
: scratch-buffer [ 04 + ] lit        ; inl

: table-size                     200 ; inl
: table>count ( addr -- addr' )      ; inl
: table>data  ( addr -- addr' ) 04 + ; inl

\ size of the global buffer
: scratch-buffer-size          10040 ; inl

wordbuf table-buffer !

\ initialize the global buffer
0 scratch-buffer buf>here !
0 scratch-buffer buf>start !
0 scratch-buffer buf>off !
0 scratch-buffer buf>end !

\ initializes the scratch-buffer
: hash_initialize ( -- )
  [ onboot @ ] lit exec
  scratch-buffer-size alloc
  dup [ scratch-buffer buf>here ] lit !
  dup [ scratch-buffer buf>start ] lit !
  scratch-buffer-size +
  [ scratch-buffer buf>end ] lit !
  scratch-buffer table-buffer !
  ;
last @ >xt onboot !

public
\ computes the hash of a counted string
: hash ( c-str n -- hash )
  0 >r
  begin
    dup if
      over c@
      r> 1036685 * + >r
      1/str again
    then
  end
  2drop r>
  ;

private
\ finds the address of the slot containing a given hash
: find-slot ( hash table -- addr )
  over >r
  dup >r table>data swap
  table-size
  tuck u/mod drop
  3 shl r@ table>data +
  swap 3 shl r> table>data +    \ d: start pos end | r: hash
  begin
    over 4 + @
    if
      over @ r@ <>
      if
         swap 8 + swap
         2dup =
         if
            nip over swap
         then
         again
      then
    then
  end
  drop nip
  r> over !                     \ write the hash
  ;

\ increments the counter of the hash table
: increment-count ( table -- err? )
  table>count dup @
  dup table-size u<
  if 1+ swap ! 0 exit then
  2drop 1
  ;

\ looks up a given string in the hash table
: table-lookup ( c-str n dict -- addr )
  dict>index @ >r 2dup hash r> find-slot
  4 + @                         \ d: c-str n addr
  >r r@ >name compare r> swap
  if drop 0 then
  ;
\ ' table-lookup is (index-lookup)

\ checks if the given hash is occupied
: table-slot ( addr table -- slot )
  >r >name hash r> find-slot tail
  ; noexit

\ set the value of a slot
: slot-set ( addr slot table -- )
  over 4 + @
  if
    drop
  else
    increment-count
    if 2drop exit then
  then
  4 + !
  ;

\ inserts a given string in the hash table
: table-insert ( addr dict -- )
  dict>index @ >r dup r@
  table-slot                    \ d: addr slot | r: table
  r> slot-set tail
  ; noexit
\ ' table-insert is (index-insert)

\ creates a new hash table
: table-create ( -- table )
  table-buffer @ >r
  r@ %align
  r@ buf>here @
  0 r@ %,
  table-size
  begin
    dup if
      0 r@ %,
      0 r@ %,
      1- again
    then
  end
  drop rdrop
  ;

public

\ builds the index of a dictionary
: build-index ( dict -- )
  table-create >r
  r@ over dict>index !
  dict>last @
  begin
    dup if
      dup r@ table-slot
      dup 4 + @ =0
      if 2dup r@ slot-set then
      drop
      >link again
    then
  end
  drop rdrop
  ;

}scope

