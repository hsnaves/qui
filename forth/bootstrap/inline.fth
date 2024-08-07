\ inline words

base c@ hex

\ word for handling dictionaries
: dict>next ( addr -- addr )       ; inl
: dict>last ( addr -- addr )  04 + ; inl
: dict>code ( addr -- addr )  08 + ; inl
: dict>data ( addr -- addr )  0C + ; inl

\ words for handling buffers
: buf>here ( addr -- addr )        ; inl
: buf>start ( addr -- addr )  04 + ; inl
: buf>end ( addr -- addr )    08 + ; inl
: buf>off ( addr -- addr )    0C + ; inl

\ flag words
: EXT    80 ; inl
: IMM    40 ; inl
: INL    20 ; inl
: LINK   10 ; inl
: XT     08 ; inl

base c! \ restore base
