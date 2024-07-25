\ inline words

base c@ hex

\ last word of dictionary
: dict>next ( addr -- addr )       ; inl
\ last word of dictionary
: dict>last ( addr -- addr )  04 + ; inl
\ buffer for code in dictinary
: dict>code ( addr -- addr )  08 + ; inl
\ buffer for data in dictionary
: dict>data ( addr -- addr )  0C + ; inl

\ obtains the here pointer of the buffer
: buf>here ( addr -- addr )        ; inl
\ obtains the start pointer of the buffer
: buf>start ( addr -- addr )  04 + ; inl
\ obtains the end pointer of the buffer
: buf>end ( addr -- addr )    08 + ; inl
\ obtains the offset pointer of the buffer
: buf>off ( addr -- addr )    0C + ; inl

\ flag words
: EXT    80 ; inl
: IMM    40 ; inl
: INL    20 ; inl
: LINK   10 ; inl
: XT     08 ; inl

base c! \ restore base
