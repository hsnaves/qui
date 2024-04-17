\ Implementation of deferred words for meta-compiler
hex

scope{
ephemeral
( *** variables for scope implementation *** )
: temp        7C ; inl
: tmpbuf      90 ; inl

private
\ obtains the xt of the exec word
: exec-xt ( -- xt )
  " exec"
  swap addr>meta swap
  0 lookup
  >xt tail
  ; noexit

public
( *** redefine variables in meta scope *** )
: temp        temp     ; inl
: tmpbuf      tmpbuf   ; inl

\ compile defer-ptr in meta dictionary
\ this is used by scopeimpl.fth
meta
