\ implementation of hash tables
hex

scope{
auxiliary
: htable-size ( addr -- addr' )       ; inl
: htable-count ( addr -- addr' ) 04 + ; inl
: htable-table ( addr -- addr' ) 08 + ; inl

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
: hfind-slot ( hash htable -- addr )
  over >r                       \ d: hash htable | r: hash
  dup >r htable-table swap      \ d: start hash | r: hash htable
  r@ htable-size @              \ d: start hash size | r: hash htable
  tuck u/mod drop               \ d: start size idx | r: hash htable
  3 shl r@ htable-table +       \ d: start size pos | r: hash htable
  swap 3 shl r> htable-table +  \ d: start pos end | r: hash
  begin
    over @ 4 + @                \ d: start pos end hasword? | r: hash
    if
      over @ r@ <>              \ d: start pos end diff? | r: hash
      if
         swap 8 + over =        \ d: start end pos' end? | r:hash
         if
            drop over swap      \ d: start pos' end | r:hash
            again
         then
      then
    then
  end
  drop nip rdrop
  ;

: hincrement ( htable -- )
  htable-count dup @
  1+ swap !
  ;

public
: hfull? ( htable -- full? )
  dup htable-count @
  swap htable-size @
  u< =0
  ;

: hlookup ( c-str n htable -- v )
  >r 2dup hash r>               \ d: c-str n hash htable
  hfind-slot                    \ d: c-str n addr
  4 + @                         \ d: c-str n val
  nip nip
  ;

: hinsert ( c-str n v htable -- ok? )
  dup hfull?                    \ d: c-str n v htable full?
  if 2drop 2drop 0 exit then    \ d: 0

  swap >r >r 2dup hash r@       \ d: c-str n hash htable | r: v htable
  hfind-slot                    \ d: c-str n addr | r: v htable

  dup 4 + @                     \ d: c-str n addr old | r: v htable
  if
    rdrop                       \ d: c-str n addr | r: v
  else
    r> hincrement               \ d: c-str n addr | r: v
  then
  r> swap 4 + !                 \ d: c-str n
  2drop 1 exit
  ;
}scope
