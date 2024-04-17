\ implementation of hash tables for dictionaries
hex

scope{
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

\ word to create a variable
: var ( size -- )
  allot create lit, wrapup
  inl tail
  ; noexit

\ variable space
align 0C buf>end allot

ephemeral
: num-tables [ dup ] lit  ; inl
: indices [ 4 + dup ] lit ; inl
: main-buffer [ 4 + ] lit ; inl

: max-tables                                         10 ; inl
: table-capacity                                    200 ; inl
: table>count ( addr -- addr' )                         ; inl
: table>dict ( addr -- addr' )                      4 + ; inl
: table>data  ( addr -- addr' )                     8 + ; inl
: table-bytes [ table-capacity 3 shl table>data ]   lit ; inl
: main-buffer-size [ table-bytes 4 + max-tables * ] lit ; inl

private

\ initialize the variables
0 main-buffer buf>off !

\ initializes the main-buffer
: allocate-main-buffer ( -- )
  main-buffer-size alloc
  dup [ main-buffer buf>here ] lit !
  dup [ main-buffer buf>start ] lit !
  dup main-buffer-size +
  [ main-buffer buf>end ] lit !
  indices ! 0 num-tables !
  [ max-tables 2 shl ] lit
  main-buffer %allot drop
  ;

allocate-main-buffer

\ initializes the table
: table-initialize ( -- )
  [ onboot @ ] lit exec
  allocate-main-buffer tail
  ; noexit
last @ >xt onboot !

\ fills the buffer with n zeros ( slots )
: zero-fill-slots ( buf n -- )
  swap >r
  begin
    dup if
      0 r@ %,
      1- again
    then
  end
  drop rdrop
  ;

\ creates a new hash table
: new-table ( -- table )
  num-tables @ dup max-tables u>=
  if drop 0 exit then
  main-buffer dup %align
  dup buf>here @ swap
  [ table-capacity 1 shl 1+ ] lit
  zero-fill-slots
  >r r@ over 2 shl indices @ + !
  1+ num-tables ! r>
  ;

\ finds the address of the slot containing a given hash
: find-slot ( hash table -- addr )
  over >r
  dup >r table>data swap
  table-capacity
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
  dup table-capacity u<
  if 1+ swap ! 0 exit then
  2drop 1
  ;

\ checks if the given hash is occupied
: word-slot ( addr table -- slot )
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

\ builds the index of a dictionary
: build-index ( dict -- )
  new-table
  dup =0 if swap dict>index ! exit then
  2dup table>dict !
  >r num-tables @ over dict>index !
  dict>last @
  begin
    dup if
      dup r@ word-slot
      dup 4 + @ =0
      if 2dup r@ slot-set then
      drop
      >link again
    then
  end
  drop rdrop
  ;

\ resolve the table from an index
: resolve-table-simple ( dict -- table )
  dup dict>index @
  num-tables @ over u<
  \ try to rebuild the index
  if 2drop 0 exit then
  1- 2 shl indices @ + @
  tuck table>dict @ <>
  if drop 0 then
  ;

\ resolve the table from an index
: resolve-table ( dict -- table )
  dup resolve-table-simple dup =0
  if
    drop dup build-index
    dup resolve-table-simple
  then
  nip
  ;

\ looks up a given string in the hash table
: i-lookup ( c-str n dict -- addr )
  resolve-table dup =0
  if nip nip exit then
  >r 2dup hash r> find-slot
  4 + @                         \ d: c-str n addr
  >r r@ >name compare r> swap
  if drop 0 then
  ;
' i-lookup is (i-lookup)

\ inserts a given string in the hash table
: i-insert ( addr dict -- )
  resolve-table dup =0
  if nip exit then
  >r dup r@
  word-slot                     \ d: addr slot | r: table
  r> slot-set tail
  ; noexit
' i-insert is (i-insert)

public

\ word to create a dictionary
: dictionary ( -- )
  align here @ [ 4 dict>index ] lit var
  0 over dict>last !
  0 over node>next !
  wordbuf over dict>code !
  wordbuf over dict>data !
  0 over dict>index !
  build-index tail
  ; \ noexit

}scope

1 forth dict>index !
2 internal dict>index !
3 extra dict>index !
