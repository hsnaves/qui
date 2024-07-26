\ basic compilation words
hex

\ the current stack should be
\ d: forth

scope{
brief
%" forth/bootstrap/inline.fth" include

public
\ the current data buffer
: data ( -- addr )
  current @
  dict>data @
  ;

\ the current code buffer
: code ( -- addr )
  current @
  dict>code @
  ;

private
\ detects if the buffer will overflow after allocating n bytes
0 ", buffer overflow"
: %free? ( n buf -- )
  dup buf>end @
  swap buf>here @ -
  swap u<
  if [ rot rot swap ] lit lit 2 error tail then
  ;

public
current @ swap current !

\ allocates a given number of bytes in the buffer
: allot ( n -- addr )
  code
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

\ aligns the here pointer to multiple of a cell
: align ( -- )
  code
  dup buf>here @
  dup 3 + -4 and
  swap - swap
  [ JSR skip allot ]
  drop
  ;

\ compile a value in a user-selected buffer
: , ( v -- )
  code
  4 over %free?
  buf>here tuck
  @ tuck !
  4 + swap !
  ;

\ compile a byte in a user-selected buffer
: c, ( b -- )
  code
  1 over %free?
  buf>here tuck
  @ tuck c!
  1 + swap !
  ;

\ compiles a string in a user-selected buffer
: s, ( c-str n -- )
  code
  over swap [ JSR skip allot ]
  copy tail
  ; noexit

\ current position of the code buffer
: here ( -- addr )
  code buf>here tail \ buf>here is empty, so okay to tail call
  ; noexit

\ position of last word in the current dictionary
: last ( -- addr )
  current @ dict>last
  ;

\ compilation words for literals and jumps
current !

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
0 ", large literal"
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
