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

public

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

\ looks up a given string in the hash table
: table-lookup ( c-str n table -- addr )
  >r 2dup hash r> find-slot
  4 + @                         \ d: c-str n addr
  >r r@ >name compare r> swap
  if drop 0 then
  ;

\ inserts a given string in the hash table
: table-insert ( addr table -- )
  \ dup table-full?
  \ if 2drop exit then
  over >r >r
  >name hash r@
  find-slot                     \ d: slot | r: addr table
  dup 4 + @
  if
    rdrop
  else
    r> increment-count
    if drop rdrop exit then
  then
  r> swap 4 + !
  ;

}scope
