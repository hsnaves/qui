\ basic compilation words
hex

\ the current stack should be
\ d: forth

( *** definition words for working with dictionaries *** )
\ node sibling
: node>next ( addr -- addr' )      ; inl

\ last word of dictionary
: dict>last ( addr -- addr )  04 + ; inl
\ buffer for code in dictinary
: dict>code ( addr -- addr )  08 + ; inl
\ buffer for data in dictionary
: dict>data ( addr -- addr )  0C + ; inl

( *** useful words for compilation *** )
\ the current code buffer
: code ( -- addr )
  current @
  dict>code @
  ;

\ the current data buffer
: data ( -- addr )
  current @
  dict>data @
  ;

( *** words for compiling using buffers *** )
\ obtains the here pointer of the buffer
: buf>here ( addr -- addr )       ; inl
\ obtains the start pointer of the buffer
: buf>start ( addr -- addr ) 04 + ; inl
\ obtains the end pointer of the buffer
: buf>end ( addr -- addr )   08 + ; inl
\ obtains the offset pointer of the buffer
: buf>off ( addr -- addr )   0C + ; inl

scope{
private
\ detects if the buffer will overflow after allocating n bytes
" buffer overflow"
: %free? ( n buf -- )
  dup buf>end @
  swap buf>here @ -
  swap u<
  if [ rot rot swap ] lit lit 2 error tail then
  ;

public
\ allocates a given number of bytes in the buffer
: %allot ( n buf -- addr )
  over dup 0 <
  if
    dup >r 0 swap -
    [ 0 buf>end ] lit >r
  else
    0 >r [ 0 buf>here ] lit >r
  then
  over %free?
  r> + dup @
  rot over + rot !
  r> +
  ;

\ compile a value in a user-selected buffer
: %, ( v buf -- )
  4 over %free?
  buf>here tuck
  @ tuck !
  4 + swap !
  ;

\ compile a byte in a user-selected buffer
: %c, ( b buf -- )
  1 over %free?
  buf>here tuck
  @ tuck c!
  1 + swap !
  ;

\ compiles a string in a user-selected buffer
: %s, ( c-str n buf -- )
  over swap %allot
  copy tail
  ; noexit

\ aligns the here pointer to multiple of a cell
: %align ( buf -- )
  dup buf>here @
  dup 3 + -4 and
  swap -
  swap %allot drop
  ;
}scope


\ here, last and other related words
current @ swap current !

\ current position of the code buffer
: here ( -- addr )
  code buf>here tail \ buf>here is empty, so okay to tail call
  ; noexit

\ position of last word in the current dictionary
: last ( -- addr )
  current @ dict>last
  ;

\ allocates a given number of bytes in the code buffer
\ NOTE: it is okay to use "here" instead of "code" bellow
: allot ( n -- addr ) here %allot tail ; noexit

\ compile a value in the code buffer
: , ( v -- )  here %, tail ; noexit

\ compile a byte in the code buffer
: c, ( b -- ) here %c, tail ; noexit

\ compiles a string in the code buffer
: s, ( c-str n -- ) here %s, tail ; noexit

\ aligns the here pointer to multiple of a cell
: align ( -- ) here %align tail ; noexit


\ compilation words for literals and jumps
current !

scope{
private
\ computes the relative address for jumps
: reladdr ( target n -- v n )
  swap over here @ + 1 + - swap
  ;

\ checks if a given value fits a literal of fixed size
\ note: the literal might be a target address
: fit? ( v n a? -- b )
  if reladdr then
  7 * 1 -
  over swap
  20 swap - dup >r shl r> shr
  = =0
  ;

\ obtains the size of a literal
: get-size ( v a? -- v n )
  >r 1
  begin
    2dup 0 r@ fit?
    if 1 + again then
  end
  r> drop
  ;

\ shows an error for large literal string
" large literal"
: print-large-error ( -- )
  [ swap ] lit lit
  1 error tail
  ; noexit

\ compile a literal of a given fixed size
: compile-literal ( v n a?-- )
  if reladdr then
  2dup 0 fit?
  if print-large-error tail then
  begin
    1 - dup =0
    if
      drop
      3F and 80 +
      c, tail
    then
    over 7 shr
    swap recurse
    7F and c, tail
  end
  ; noexit

public
\ compile a literal
: l, ( v -- )
  0 get-size 0 compile-literal tail
  ; noexit

\ compiles a literal for jump of automatic size
\ based on jsz
: lj, ( target -- )
  jsz c@ 1 compile-literal tail
  ; noexit

\ compiles a jump to a target
: j, ( target insn -- )
  >r 1 get-size 1 compile-literal r> c, tail
  ; noexit

brief
include" forth/bootstrap/flags.fth"

public
( define constants for control instructions )
: RET    C0 ; inl
: JSR    C1 ; inl
: JMP    C2 ; inl
: JZ     C3 ; inl

( public flags )
: IMM    IMM ; inl
: INL    INL ; inl
}scope
