\ global variables
hex

( define global variables )
: currnext ( -- addr ) 04 ; inl
: current ( -- addr )  08 ; inl
: context ( -- addr )  0C ; inl
: this ( -- addr )     10 ; inl
: onexcept ( -- addr ) 14 ; inl
: onboot ( -- addr )   18 ; inl
: flags ( -- addr )    1C ; inl
: state ( -- addr )    1D ; inl
: base ( -- addr )     1E ; inl
: janum ( -- addr )    1F ; inl
: forth ( -- addr )    20 ; inl
: internal ( -- addr ) 34 ; inl
: wordbuf ( -- addr )  48 ; inl
: tib ( -- addr )      58 ; inl
