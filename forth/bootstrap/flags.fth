\ flag words
base c@ hex

( define constants for flags )
: F_EXT    80 ; inl
: F_IMM    40 ; inl
: F_INL    20 ; inl
: F_LINK   10 ; inl
: F_XT     08 ; inl

base c! \ restore base


