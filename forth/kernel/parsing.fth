\ implementation of parsing words
hex

( *** words related to the TIB *** )

scope{
public
\ memory allocation function with validation
\ if it returns, the address is guarateed to be non-zero
\ otherwise it terminates the program
," memory exhausted"
: allocate ( size -- addr )
  alloc dup if exit then
  drop 1 channel c!
  [ swap ] lit lit type nl
  type nl
  1 terminate tail
  ; noexit

auxiliary
: TIB_SIZE 1000 ; inl

private
\ initializes the tib
: tib_initialize ( -- )
  [ onboot @ ] lit exec
  TIB_SIZE allocate
  dup [ tib buf>here ] lit !
  dup [ tib buf>start ] lit !
  dup [ tib buf>off ] lit !
  TIB_SIZE + [ tib buf>end ] lit !
  ;
last @ >xt onboot !
}scope

scope{
auxiliary
\ checks for a newline
: nl? ( c -- b ) 0A = ; inl

private
\ prints an error messages that the TIB overflowed
," TIB overflow"
: tiboverflow ( -- )
  [ swap ] lit lit error tail
  ; noexit

public
\ Obtains one line from the input
: line ( -- )
  \ reset the tib
  [ tib buf>start ] lit @ dup
  [ tib buf>here ] lit !
  [ tib buf>off ] lit !
  begin
    getc [ tib buf>here ] lit @
    2dup c!
    1+ [ tib buf>here ] lit !
    nl? =0
    if
      [ tib buf>here ] lit @
      [ tib buf>end ] lit @
      u< if again then
      tiboverflow tail
    then
  end
  ;
}scope

scope{
private
\ ensures that the TIB has some character
: ensuretib ( -- )
  begin
    [ tib buf>off ] lit @
    [ tib buf>here ] lit @
    u>= if line again then
  end
  ;

public

\ Obtains a character from the TIB
: key ( -- c )
  ensuretib
  [ tib buf>off ] lit
  dup @ tuck 1+ swap !
  c@
  ;

}scope

scope{
auxiliary
\ checks if a character is blank
: blank? ( c -- b )
  [ char ! ] lit u<
  ; inl

public
\ Obtains a word from the TIB
: word ( -- c-str n )
  \ skip blanks
  begin
    key blank?
    if again then
  end
  [ tib buf>off ] lit @
  dup 1- swap
  [ tib buf>here ] lit @
  begin                         \ d: c-str voffset vhere
    2dup u<
    if
      over c@ blank? =0
      if swap 1+ swap again then
    then
  end
  drop dup 1+
  [ tib buf>off ] lit !
  over -
  ;
}scope

( *** implementation of the compare word *** )
\ compare two counted strings
\ returns true when not equal
: compare ( c-str1 n1 c-str2 n2 -- neq? )
  swap >r over <>
  if 2drop r> drop 1 exit then

  begin \ d: c-str1 n1 | r: c-str2
    dup if
      over c@ r@ c@ =
      if 1/str r> 1+ >r again then
    then
  end
  nip rdrop
  ;

( *** implementation of the number word *** )
scope{
private
\ character to digit word
: c>d ( c -- dig )
  dup [ char 9 char 0 ] lit lit
  within if [ char 0 ] lit - exit then
  dup [ char Z char A ] lit lit
  within =0 if drop -1 exit then
  [ char A 0A - ] lit -
  ;

\ counted string to unsigned number
\ returns the parsed number together with the number of
\ remaining characters in the counted string
: unumber ( c-str n -- u rem )
  swap 0
  begin  \ d: n c-str u
    over c@ c>d
    base c@ 2dup u<
    if                          \ d: n c-str u dig vbase
      rot * + rot 1-
      >r r@ rot 1+ rot r>       \ d: n c-str' u n
      =0 until
      2dup
    then
    2drop
  end
  nip swap
  ;

public
\ counted string to signed number
\ returns the parsed number and the number of remaining
\ characters in the counted string
: number ( c-str n -- num rem )
  1 over u<
  if
    over c@ [ char - ] lit  =
    if
      1/str unumber
      0 rot - swap
      exit
    then
  then
  unumber tail
  ; noexit
}scope

\ prints an error message of an unknown word
," ? "
: unknown ( c-str n -- )
  [ swap ] lit lit
  1 channel c! type
  error tail
  ; noexit

