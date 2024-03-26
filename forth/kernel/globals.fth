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
: channel ( -- addr )  1F ; inl
: forth ( -- addr )    20 ; inl
: internal ( -- addr ) 30 ; inl
: wordbuf ( -- addr )  40 ; inl
: tib ( -- addr )      50 ; inl
