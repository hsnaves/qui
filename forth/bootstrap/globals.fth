\ global variables
hex

\ the current stack should be
\ d: forth inner

( define global variables )
current !
: other ( -- addr )    04 ; inl
: ontrap ( -- addr )   14 ; inl
: onboot ( -- addr )   18 ; inl
: dfl ( -- addr )      1C ; inl
: jsz ( -- addr )      1F ; inl
: wbuf ( -- addr )     40 ; inl
: tib ( -- addr )      50 ; inl
current !
: current ( -- addr )  08 ; inl
: context ( -- addr )  0C ; inl
: this ( -- addr )     10 ; inl
: state ( -- addr )    1D ; inl
: base ( -- addr )     1E ; inl
: forth ( -- addr )    20 ; inl
: inner ( -- addr )    30 ; inl
