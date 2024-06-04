\ implementation of hash tables for dictionaries
decimal

scope{
public
\ computes the hash of a counted string
: hash ( c-str n -- hash )
  0 >r
  begin
    dup if
      over c@
      r> 17000069 * + >r
      1/str again
    then
  end
  2drop r>
  ;

\ word to embed a constant
: const ( v -- )
  create lit, wrapup inl tail
  ; noexit

\ variable space
align 12 buf>off allot

ephemeral
: num-tables [ dup ] lit  ; inl
: indices [ 4 + dup ] lit ; inl
: main-buffer [ 4 + ] lit ; inl

: max-tables           16 ; inl
: table-capacity      512 ; inl \ must be a power of 2
: table>count             ; inl
: table>dict          4 + ; inl
: table>data          8 + ; inl
: entry>hash              ; inl
: entry>value         4 + ; inl
: next-entry          8 + ; inl

: table-bytes [ table-capacity 3 shl table>data ] lit ; inl
: main-buffer-size [ table-bytes 4 + max-tables * ] lit ; inl

private
\ initialize the variables
0 main-buffer buf>off !

\ initializes the main-buffer
: allocate-main-buffer ( -- )
  main-buffer-size alloc
  dup indices !
  dup [ main-buffer buf>start ] lit !
  dup main-buffer-size +
  [ main-buffer buf>end ] lit !
  [ max-tables 2 shl ] lit +
  [ main-buffer buf>here ] lit !
  0 num-tables !
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
  [ table-bytes 2 ushr ] lit
  zero-fill-slots
  >r r@ over 2 shl indices @ + !
  1+ num-tables ! r>
  ;

\ finds the address of the slot containing a given hash
: find-slot ( hash table -- slot )
  over >r
  dup >r table>data swap
  table-capacity tuck 1- and
  3 shl r@ table>data +
  swap 3 shl r> table>data +    \ d: start pos end | r: hash
  begin
    over entry>value @ if
      over entry>hash @ r@ <> if
        swap next-entry swap
        2dup = if
          nip over swap
        then
        again
      then
    then
  end
  drop nip r> over entry>hash !
  ;

\ increments the counter of the hash table
: increment-count ( table -- err? )
  table>count dup @ dup
  [ table-capacity 1- ] lit u<  \ always keep 1 slot free to avoid
                                \ infinite loops in find-slot
  if 1+ swap ! 0 exit then
  2drop 1
  ;

\ checks if the given hash is occupied
: word-slot ( table addr -- slot )
  >name hash swap find-slot tail
  ; noexit

\ set the value of a slot
: slot-set ( addr slot table -- )
  over entry>value @
  if
    drop
  else
    increment-count
    if 2drop exit then
  then
  entry>value !
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
      r@ over word-slot
      dup entry>value @ =0
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
  if 2drop 0 exit then
  1- 2 shl indices @ + @
  tuck table>dict @ = and
  ;

\ resolve the table from an index
: resolve-table ( dict -- table )
  dup resolve-table-simple dup =0
  if
    \ try to rebuild the index
    drop dup build-index
    dup resolve-table-simple
  then
  nip
  ;

\ looks up a given string in the hash table
: i-lookup ( c-str n dict -- c-str n addr )
  resolve-table
  dup =0 if exit then
  >r 2dup hash r> find-slot
  entry>value @                 \ d: c-str n addr
  >r 2dup r@ >name compare
  =0 r> and
  ;
' i-lookup is (i-lookup)

\ inserts a given string in the hash table
: i-insert ( addr dict -- )
  resolve-table dup =0
  if nip exit then
  >r r@ over
  word-slot                     \ d: addr slot | r: table
  r> slot-set tail
  ; noexit
' i-insert is (i-insert)

public
\ word to create a dictionary
: dictionary ( -- )
  align [ 4 dict>index ] lit
  allot dup const
  0 over dict>last !
  0 over node>next !
  wordbuf over dict>code !
  wordbuf over dict>data !
  max-tables swap dict>index !
  ;

internal current !
\ drop the last table from the index
: (drop-table) ( -- )
  num-tables @
  dup =0 if drop exit then
  1- num-tables !
  [ main-buffer buf>here ] lit
  dup @ table-bytes - swap !
  ;
forth current !
}scope

1 forth dict>index !
2 internal dict>index !
3 extra dict>index !
