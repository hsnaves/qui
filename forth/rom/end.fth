\ last words of rom
hex

scope{
private
\ handles general exceptions
: general_exception ( status c-str n -- )
  0 error 0 r@              \ d: status pc
  dup . space
  c@ . nl
  status! tail
  ; noexit

\ handles invalid instructions
" invalid instruction at "
: invalid_insn ( status -- )
  [ swap ] lit lit
  general_exception tail
  ; noexit

\ handles divide by zero errors
" divide by zero at "
: divide_by_zero ( status -- )
  [ swap ] lit lit
  general_exception tail
  ; noexit

\ current implementation of onexcept
: my_onexcept ( status -- )
  dup 1 = if
    invalid_insn tail
  then
  dup 2 = if
    divide_by_zero tail
  then
  status! tail
  ; noexit
last @ >xt onexcept !

\ default prompt implementation
" 
> "
: default_prompt ( -- )
  state c@ if exit then \ not show prompt when compiling
  channel 1 + c@ if exit then \ not show when not from stdin
  [ find getc defer-ptr ] lit @
  [ ' (getc) ] lit - if exit then
  [ swap ] lit lit type tail
  ; noexit
last @ >xt

\ new onboot
: check-args ( -- )
  [ onboot @ ] lit exec
  channel 1 + 1 over c! line
  0 swap c! \ restore input
  [ tib buf>off ] lit @
  [ tib buf>here ] lit @
  dup [ tib buf>off ] lit !
  over - 1 - dup if
    2dup here @ >r
    str, 0 c, r> (include)
  then
  drop drop
  ;
last @ >xt onboot !
}scope

decimal 1 jsize c!
stg-name" main.rom"
is prompt 0 0 here @ 2 stg-do . nl bye

