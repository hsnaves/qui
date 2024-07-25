\ last words of rom
hex

private
\ handles general exceptions
" trap at "
: general_trap ( status -- )
  [ swap ] lit lit 0 error
  0 r@ . space
  sts! tail
  ; noexit
last @ >xt ontrap !

\ new onboot
: check-args ( -- )
  [ onboot @ ] lit exec
  chn 1 + 1 over c! line
  0 swap c! \ restore input
  [ tib buf>off ] lit @
  [ tib buf>here ] lit @
  dup [ tib buf>off ] lit !
  over - 1 - dup if
    2dup here @ >r
    s, 0 c, r> (include)
  then
  drop drop
  ;
last @ >xt onboot !
}scope

decimal 1 jsz c!
f-name" kernel.rom"
0 0 here @ 2 f-do . nl bye

