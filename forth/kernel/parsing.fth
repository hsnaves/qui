\ implementation of parsing words
hex

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
    drop drop
  end
  r> swap rot drop
  ;

\ prints an error message of an unknown word
" ? "
: unknown ( c-str n -- )
  [ swap ] lit lit 0 error 1 error tail
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

public
\ like (number) but with validation
: number ( c-str n -- num )
  2dup (number)
  if drop unknown tail then
  nip nip
  ;

include" forth/kernel/find.fth"
}scope
