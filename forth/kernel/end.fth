\ last words of rom
hex

scope{
private
\ handles general exceptions
" exception at "
: general_exception ( status -- )
  [ swap ] lit lit 0 error
  0 r@ . space
  status! tail
  ; noexit
last @ >xt onexcept !

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
stg-name" kernel.rom"
0 0 here @ 2 stg-do . nl bye

