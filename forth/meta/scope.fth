\ Implementation of deferred words for meta-compiler

hex

scope{
auxiliary

( *** variables for scope implementation *** )
: temp        60 ; inl
: tmpbuf      70 ; inl
: templink    80 ; inl
: tempcurr    84 ; inl

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

( *** redefine variables in meta scope *** )
: temp        temp     ; inl
: tmpbuf      tmpbuf   ; inl
: templink    templink ; inl
: tempcurr    tempcurr ; inl
