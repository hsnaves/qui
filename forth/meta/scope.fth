\ Implementation of deferred words for meta-compiler
hex

scope{
auxiliary
( *** variables for scope implementation *** )
: temp        68 ; inl
: tmpbuf      7C ; inl
: tnode       8C ; inl
: tcurr       90 ; inl

private
\ obtains the xt of the exec word
," exec"
: exec-xt ( -- xt )
  [ swap ] lit lit
  swap addr>meta swap
  0 lookup
  >xt tail
  ; noexit

public
( *** redefine variables in meta scope *** )
: temp        temp     ; inl
: tmpbuf      tmpbuf   ; inl
: tnode       tnode    ; inl
: tcurr       tcurr    ; inl
