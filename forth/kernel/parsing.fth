\ implementation of parsing words
hex

scope{
private
\ shrinks wordbuf and returns the address of the
\ reserved memory following the shrunk buffer
\ returns zero when it fails
: (alloc) ( size -- addr )
  [ wordbuf buf>end ] lit @ tuck
  [ wordbuf buf>here ] lit @ -
  over u< if 2drop 0 exit then
  - dup [ wordbuf buf>end ] lit !
  ;

public
\ memory allocation function with validation
\ if it returns, the address is guarateed to be non-zero
\ otherwise it terminates the program
" memory exhausted"
: alloc ( size -- addr )
  (alloc) dup if exit then
  drop [ swap ] lit lit fatal tail
  ; noexit

ephemeral
: TIB_SIZE 1000 ; inl

private
\ initializes the tib
: tib_initialize ( -- )
  [ onboot @ ] lit exec
  TIB_SIZE alloc
  dup [ tib buf>here ] lit !
  dup [ tib buf>start ] lit !
  dup [ tib buf>off ] lit !
  TIB_SIZE + [ tib buf>end ] lit !
  ;
last @ >xt onboot !
}scope

scope{
ephemeral
: eof? ( c -- b ) 0 < ; inl
: nl? ( c -- b ) 0A = ; inl

private
" TIB overflow"
: tiboverflow ( -- )
  [ swap ] lit lit error tail
  ; noexit

public
\ Obtains one line from the input
: line ( -- )
  prompt
  [ tib buf>end ] lit @ >r
  [ tib buf>start ] lit @
  dup [ tib buf>here ] lit !
  dup [ tib buf>off ] lit !
  begin
    getc swap 2dup c! 1+
    over eof? if bye tail then
    swap nl? =0
    if
      dup r@ u< if again then
      tiboverflow tail
    then
  end
  [ tib buf>here ] lit !
  rdrop
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
  dup @ tuck 1+ swap ! c@
  ;
}scope

scope{
ephemeral
: blank? ( c -- b )
  [ char ! ] lit u<
  ; inl

public
\ Obtains a word from the TIB
: word ( -- c-str n )
  begin
    key blank?
    if again then
  end
  [ tib buf>off ] lit @
  dup 1- swap
  [ tib buf>here ] lit @ >r
  begin
    dup r@ u<
    if
      dup c@ blank? =0
      if 1+ again then
    then
  end
  rdrop dup 1+
  [ tib buf>off ] lit !
  over -
  ;
}scope

scope{
private
\ character to digit word
: c>d ( c -- dig )
  dup [ char 9 char 0 ] lit lit
  within if [ char 0 ] lit - exit then
  dup [ char Z char A ] lit lit
  within if [ char A 0A - ] lit - exit then
  drop -1
  ;

\ counted string to unsigned number
: unumber ( c-str n -- u rem )
  0 >r
  begin
    over c@ c>d
    base c@ 2dup u<
    if \ d: c-str n dig vbase | r: u
      r> * + >r
      1/str dup =0 until
      2dup
    then
    2drop
  end
  r> swap rot drop
  ;

public
internal current !
\ prints an error message of an unknown word
" ? "
: unknown ( c-str n -- )
  ch_err [ swap ] lit lit type
  error tail
  ; noexit

\ counted string to signed number
\ returns the parsed number together with the number of
\ remaining characters in the counted string
\ this word assumes n is positive
: (number) ( c-str n -- num rem )
  1 over u<
  if
    over c@ [ char - ] lit  =
    if
      1/str unumber
      0 rot - swap exit
    then
  then
  unumber tail
  ; noexit
forth current !

\ like (number) but with validation
: number ( c-str n -- num )
  2dup (number)
  if drop unknown tail then
  nip nip
  ;
}scope
