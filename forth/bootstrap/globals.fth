\ global variables
hex

\ the current stack should be
\ d: forth internal

( define global variables )
current !
: currnext ( -- addr ) 04 ; inl
: onexcept ( -- addr ) 14 ; inl
: onboot ( -- addr )   18 ; inl
: flags ( -- addr )    1C ; inl
: jsize ( -- addr )    1F ; inl
: wordbuf ( -- addr )  40 ; inl
: tib ( -- addr )      50 ; inl
current !
: current ( -- addr )  08 ; inl
: context ( -- addr )  0C ; inl
: this ( -- addr )     10 ; inl
: state ( -- addr )    1D ; inl
: base ( -- addr )     1E ; inl
: forth ( -- addr )    20 ; inl
: internal ( -- addr ) 30 ; inl
