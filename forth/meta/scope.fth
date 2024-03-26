\ Implementation of deferred words for meta-compiler

hex

( *** variables for scope implementation *** )
: currnext    04 ; inl
: temp        60 ; inl
: tmpbuf      70 ; inl
: templink    80 ; inl
: tempcurr    84 ; inl

meta use
tmpbuf buf>end
tmpbuf buf>off
tmpbuf buf>start
tmpbuf buf>here
temp dict>data
temp dict>code
temp dict>last
temp dict>next
meta-exit

: tempnext    lit ; inl
: templast    lit ; inl
: tempcode    lit ; inl
: tempdata    lit ; inl
: tmpbufhere  lit ; inl
: tmpbufstart lit ; inl
: tmpbufoff   lit ; inl
: tmpbufend   lit ; inl


scope{
private

\ obtains the xt of the exec word
," exec"
: exec-xt ( -- xt )
   [ swap ] lit lit             \ d: c-str n
   swap addr>meta swap          \ d: c-str' n
   0 lookup                     \ d: addr
   >xt tail                     \ d: xt
   ; noexit

public

