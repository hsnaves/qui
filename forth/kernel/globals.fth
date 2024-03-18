\ global variables

hex

( define global variables )
: current ( -- addr )  08 ; inl
: context ( -- addr )  0C ; inl
: this ( -- addr )     10 ; inl
: flags ( -- addr )    14 ; inl
: state ( -- addr )    15 ; inl
: base ( -- addr )     16 ; inl
: channel ( -- addr )  17 ; inl
: forth ( -- addr )    20 ; inl
: compiler ( -- addr ) 30 ; inl
: wordbuf ( -- addr )  40 ; inl
: tib ( -- addr )      50 ; inl
