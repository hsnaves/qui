\ Implementation of deferred words for meta-compiler
hex

scope{
ephemeral
( *** variables for scope implementation *** )
: temp        7C ; inl
: tmpbuf      90 ; inl

public
( *** redefine variables in meta scope *** )
: temp        temp     ; inl
: tmpbuf      tmpbuf   ; inl

\ compile defer-ptr in meta dictionary
\ this is used by scopeimpl.fth
meta
