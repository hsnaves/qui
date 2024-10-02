decimal

scope{
private
align 40 allot
brief
: input-filename  [ dup ]     lit ; inl
: input-namelen   [ 4 + dup ] lit ; inl
: input-offset    [ 4 + dup ] lit ; inl
: input-length    [ 4 + dup ] lit ; inl
: input-pos       [ 4 + dup ] lit ; inl
: output-filename [ 4 + dup ] lit ; inl
: output-namelen  [ 4 + dup ] lit ; inl
: output-offset   [ 4 + dup ] lit ; inl
: output-length   [ 4 + dup ] lit ; inl
: prev-emit       [ 4 + ]     lit ; inl
: buffer-size                  64 ; inl

private
align 128 allot
brief
: input-buffer    [ dup ]     lit ; inl
: output-buffer   [ 64 + ]    lit ; inl

private
: reset-vars ( -- )
  0 input-offset !
  0 output-offset !
  0 output-length !
  ;

: restore-prev-emit ( -- )
  prev-emit @
  [ find emit >dptr ] lit !
  ;

: error-fileio ( c-str n -- )
  restore-prev-emit
  " error in file operation: " 0 error
  2 error tail
  ; noexit

: flush-output ( -- )
  output-filename @
  output-namelen @ f-name!
  output-offset @
  output-buffer
  output-length @ f-data!
  2 f-do
  dup 0 < if
    output-filename @
    output-namelen @ error-fileio tail
  then
  output-offset @ + output-offset !
  0 output-length !
  ;

: new-emit ( c -- )
  output-length @ swap over
  output-buffer + c!
  1 + dup output-length !
  buffer-size =
  if flush-output then
  ;

: install-new-emit ( -- )
  [ ' new-emit ] lit
  [ find emit >dptr ] lit !
  ;

: refresh-input ( -- )
  input-filename @
  input-namelen @ f-name!
  input-offset @
  input-buffer buffer-size f-data!
  1 f-do
  dup 0 < if
    input-filename @
    input-namelen @ error-fileio tail
  then
  dup input-offset @ + input-offset !
  input-length !
  0 input-pos !
  ;

: available? ( -- ok? )
  input-length @
  ;

: fetch-character ( -- c )
  input-pos @ dup
  input-buffer + c@
  swap 1 + dup input-pos !
  input-length @ =
  if refresh-input then
  ;

\ prints a given number of spaces
: spaces ( n -- )
  begin
    dup if
      space 1 -
      again
    then
  end
  drop
  ;

\ print a byte to the output in hexadecimal (no space)
: b. ( v -- )
  dup 4 ushr
  15 and d>c emit
  15 and d>c emit tail
  ; noexit

: dump-entry ( -- )
  " 0x" type
  fetch-character b.
  " ," type tail
  ; noexit

: dump-row ( -- )
  3 spaces 8
  begin
    dup if
      1 -
      available? if
        space dump-entry
        again
      then
    then
  end
  drop
  nl tail
  ; noexit

0 ", /* auto-generated binary code (do not edit it) */

static const uint8_t kernel_rom[] = {
"
: dump-header ( -- )
  [ swap ] lit lit type tail
  ; noexit

: dump-footer ( -- )
  " };

" type tail
  ; noexit

public

: dump-data ( -- )
  1 chn 1 + c! " input: " type
  here @ input-filename !
  word 2dup type nl
  dup input-namelen ! s,
  " output: " type
  here @ output-filename !
  word 2dup type nl
  dup output-namelen ! s,

  reset-vars
  install-new-emit
  refresh-input
  dump-header
  begin
    available? if
      dump-row
      again
    then
  end
  dump-footer
  flush-output
  restore-prev-emit
  bye tail
  ; noexit

}scope

dump-data
