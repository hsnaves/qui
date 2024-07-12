\ implementation of find word

\ finds the next word from TIB in the context
: find ( -- addr )
  word 0 lookup
  dup if nip nip exit then
  drop unknown tail
  ; noexit
