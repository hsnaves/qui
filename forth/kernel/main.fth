\ basic words
hex

( basic words )
: =0    ( n -- {n==0} )            [ C4 c, ] ; inl
: =     ( n1 n2 -- {n1==n2} )      [ C5 c, ] ; inl
: u<    ( u1 u2 -- {u1<u2} )       [ C6 c, ] ; inl
: <     ( n1 n2 -- {n1<n2} )       [ C7 c, ] ; inl
: and   ( n1 n2 -- {n1&n2} )       [ C8 c, ] ; inl
: or    ( n1 n2 -- {n1|n2} )       [ C9 c, ] ; inl
: xor   ( n1 n2 -- {n1^n2} )       [ CA c, ] ; inl
: +     ( n1 n2 -- {n1+n2} )       [ CB c, ] ; inl
: -     ( n1 n2 -- {n1-n2} )       [ CC c, ] ; inl
: ?:    ( n1 n2 n3 -- {n3?n2:n1} ) [ CD c, ] ; inl
: du*   ( u1 u2 -- lo high )       [ CE c, ] ; inl
: u/mod ( u1 u2 -- rem quot )      [ CF c, ] ; inl
: @     ( addr -- n )              [ D0 c, ] ; inl
: !     ( n addr -- )              [ D1 c, ] ; inl
: c@    ( addr -- c )              [ D2 c, ] ; inl
: c!    ( c addr -- )              [ D3 c, ] ; inl
: signe ( c -- n )                 [ D4 c, ] ; inl
: shl   ( n1 n2 -- {n1<<n2} )      [ D5 c, ] ; inl
: ushr  ( u1 u2 -- {u1>>u2} )      [ D6 c, ] ; inl
: shr   ( n1 n2 -- {n1>>n2} )      [ D7 c, ] ; inl
: nop   ( -- )                     [ D8 c, ] ; inl
: dup   ( n -- n n )               [ D9 c, ] ; inl
: drop  ( n -- )                   [ DA c, ] ; inl
: swap  ( n1 n2 -- n2 n1 )         [ DB c, ] ; inl
: over  ( n1 n2 -- n1 n2 n1 )      [ DC c, ] ; inl
: rot   ( n1 n2 n3 -- n2 n3 n1 )   [ DD c, ] ; inl

( stack manipulation words )
: >r    ( n -- r: n )              [ DE c, ] ; inl
: r>    ( r: n -- n )              [ DF c, ] ; inl
: r@    ( raddr -- n )             [ E0 c, ] ; inl
: r!    ( n raddr -- )             [ E1 c, ] ; inl

( some other composite words )
: nip   ( n1 n2 -- n2 )           swap drop  ; inl
: tuck  ( n1 n2 -- n2 n1 n2 )     swap over  ; inl
: 2dup  ( n1 n2 -- n1 n2 n1 n2 )  over over  ; inl
: *     ( n1 n2 -- {n1*n2} )       du* drop  ; inl
: not   ( n1 -- {~n1} )             -1  xor  ; inl
: bool  ( n1 -- {n1!=0} )           =0   =0  ; inl
: <>    ( n1 n2 -- {n1!=n2} )        =   =0  ; inl
: u>    ( u1 u2 -- {u1>u2} )      swap   u<  ; inl
: >     ( n1 n2 -- {n1>n2} )      swap    <  ; inl
: u>=   ( u1 u2 -- {u1>=u2} )       u<   =0  ; inl
: >=    ( n1 n2 -- {n1>=n2} )        <   =0  ; inl
: u<=   ( u1 u2 -- {u1<=u2} )       u>   =0  ; inl
: <=    ( n1 n2 -- {n1<=n2} )        >   =0  ; inl
: umod  ( u1 u2 -- rem )         u/mod drop  ; inl
: u/    ( u1 u2 -- quot )        u/mod  nip  ; inl

( *** words to change base *** )

\ change the base to hexadecimal
: hex ( -- ) 10 base c!  ;

\ change the base to decimal
: decimal ( -- ) 0A base c!  ;

\ executes a given function
: exec ( addr -- ... ) >r ;

( *** implementation of basic system words *** )

scope{
ephemeral
( *** constants for the QUI vm *** )
: IO_SYS_SELECTOR    FFFFFFF8 ; inl
: IO_SYS_VALUE       FFFFFFF4 ; inl
: SYS_STATUS                1 ; inl
: SYS_DSP                   2 ; inl
: SYS_RSP                   3 ; inl
: SYS_ISTART                8 ; inl
: SYS_IEND                  9 ; inl

public

internal current !
\ obtains the address of a system register
: sysreg ( n -- addr ) IO_SYS_SELECTOR ! IO_SYS_VALUE ;

\ set status value
: status! ( v -- ) SYS_STATUS sysreg ! ;

\ the address of the data stack pointer
: dsp ( -- addr ) SYS_DSP sysreg tail ; noexit

\ the address of the return stack pointer
: rsp ( -- addr ) SYS_RSP sysreg tail ; noexit

\ flushes the code cache
: flush ( iend istart -- )
  SYS_ISTART sysreg !
  SYS_IEND sysreg !
  ;

forth current !

\ terminate the program successfully
: bye ( -- ) 0 status! tail ; noexit

}scope

( *** helper words *** )

\ returns true if min <= v <= max
: within ( v min max -- t )
  rot tuck <
  if drop drop 0 exit then
  swap >=
  ;

\ advances the counted string by a given number of characters
\ : /str ( c-str n num -- c-str' n' )
\  rot over + rot rot -
\  ;

\ advances the counted string by 1
: 1/str ( c-str n -- c-str' n' )
  1 - swap 1 + swap
  ;

\ copies a given string into the destination buffer
: str-copy ( c-str n dst -- )
  >r
  begin
    dup if
      over c@ 0 r@ c!
      r> 1 + >r 1/str
      again
    then
  end
  drop drop r> drop
  ;

\ finds a given character in the string
\ returns the index of the character
: char-find ( c-str c -- idx )
  over
  begin
    2dup c@ <>
    if 1 + again then
  end
  rot - nip
  ;

( *** deferred words *** )

\ prompt shown before any given input line
align defer prompt ( -- )

\ prints an error message and possibly
\ restarts the interpreter or exits the program
align defer error ( c-str n severity  -- )

\ obtains an input character from standard input
align defer getc ( -- c)

\ emits one character to the standard output
align defer emit ( c -- )

( *** type word *** )

\ types a counted string to the standard output
: type ( c-str n -- )
  begin
    dup =0
    if drop drop exit then
    over c@ emit
    1/str
    again
  end
  ; noexit

( *** words related to printing *** )
\ prints space
: space ( -- )  20 emit tail ; noexit

\ prints a newline to the console
: nl ( -- ) 0A emit tail ; noexit

( *** initializes the wordbuf *** )
scope{
ephemeral
( *** constants for the QUI vm *** )
: SYS_MEMSIZE               5 ; inl

private
\ initializes the wordbuf
: wordbuf_initialize ( -- )
  SYS_MEMSIZE sysreg @ [ wordbuf buf>end ] lit !
  ;
last @ >xt onboot !
}scope
