\ Implementation of deferred words for meta-compiler
hex

scope{
brief
( *** variables for scope implementation *** )
: temp        60 ; inl
: tmpbuf      70 ; inl

public
( *** redefine variables in meta scope *** )
: temp        temp     ; inl
: tmpbuf      tmpbuf   ; inl

\ compile >dptr in meta dictionary
\ this is used by scopeimpl.fth
meta
