decimal

scope{
private
align 32 allot
ephemeral
: input-filename  [ dup ]     lit ; inl
: input-offset    [ 4 + dup ] lit ; inl
: input-length    [ 4 + dup ] lit ; inl
: input-pos       [ 4 + dup ] lit ; inl
: output-filename [ 4 + dup ] lit ; inl
: output-offset   [ 4 + dup ] lit ; inl
: output-length   [ 4 + dup ] lit ; inl
: prev-emit       [ 4 + ]     lit ; inl
: buffer-size                  64 ; inl

private
align 128 allot
ephemeral
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
  [ defer-ptr (emit) ] lit !
  ;

: error-fileio ( cstr -- )
  restore-prev-emit
  1 channel c!
  " error in file operation: " type
  dup 0 char-find over -
  fatal tail
  ; noexit

: flush-output ( -- )
  output-filename @ file-name!
  output-offset @
  output-buffer
  output-length @
  2 file-do
  dup 0 < if
    output-filename @ error-fileio tail
  then
  output-offset @ + output-offset !
  0 output-length !
  ;

: new-emit ( c -- )
  output-length @ swap over
  output-buffer + c!
  1+ dup output-length !
  buffer-size =
  if flush-output then
  ;

: install-new-emit ( -- )
  [ ' new-emit ] lit
  [ defer-ptr emit ] lit !
  ;

: refresh-input ( -- )
  input-filename @ file-name!
  input-offset @
  input-buffer buffer-size
  1 file-do
  dup 0 < if
    input-filename @ error-fileio tail
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
  swap 1+ dup input-pos !
  input-length @ =
  if refresh-input then
  ;

: dump-entry ( -- )
  " 0x" type
  fetch-character b.
  " ," type tail
  ; noexit

: dump-row ( -- )
  3 spaces 8
  begin
    dup if
      1-
      available? if
        space dump-entry
        again
      then
    then
  end
  drop
  nl tail
  ; noexit

" /* auto-generated binary code (do not edit it) */

static const uint8_t default_rom[] = {
"
: dump-header ( -- )
  [ swap ] lit lit type tail
  ; noexit

: dump-footer ( -- )
  " };

" type tail
  ; noexit

public

: dump-data ( input output -- )
  output-filename !
  input-filename !
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
  ;

}scope

" rom.bin" drop 0 c,
" src/default_rom.c" drop 0 c,

dump-data
bye

