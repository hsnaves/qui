\ basic words
hex

( basic words )
: =0    ( n -- {n==0} )            [ C4 c, ] ; inl
: =     ( n1 n2 -- {n1==n2} )      [ C5 c, ] ; inl
: u<    ( u1 u2 -- {u1<u2} )       [ C6 c, ] ; inl
: <     ( n1 n2 -- {n1<n2} )       [ C7 c, ] ; inl
: nop   ( -- )                     [ C8 c, ] ; inl
: and   ( n1 n2 -- {n1&n2} )       [ C9 c, ] ; inl
: or    ( n1 n2 -- {n1|n2} )       [ CA c, ] ; inl
: xor   ( n1 n2 -- {n1^n2} )       [ CB c, ] ; inl
: +     ( n1 n2 -- {n1+n2} )       [ CC c, ] ; inl
: -     ( n1 n2 -- {n1-n2} )       [ CD c, ] ; inl
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
: dup   ( n -- n n )               [ D8 c, ] ; inl
: drop  ( n -- )                   [ D9 c, ] ; inl
: swap  ( n1 n2 -- n2 n1 )         [ DA c, ] ; inl
: over  ( n1 n2 -- n1 n2 n1 )      [ DB c, ] ; inl
: rot   ( n1 n2 n3 -- n2 n3 n1 )   [ DC c, ] ; inl

( stack manipulation words that will be overriden latter )
F_XT flags c!
: >r    ( n -- r: n )              [ DD c, ] ; inl
: r>    ( r: n -- n )              [ DE c, ] ; inl
: r@    ( r: n -- n | r: n )       [ DF c, ] ; inl

( composite stack manipulation words that will be overriden latter )
: rdrop ( r: n -- | r: )            r>  drop ; inl
0 flags c!

( some other composite words )
: nip   ( n1 n2 -- n2 )           swap drop  ; inl
: tuck  ( n1 n2 -- n2 n1 n2 )     swap over  ; inl
: 2dup  ( n1 n2 -- n1 n2 n1 n2 )  over over  ; inl
: 2drop ( n1 n2 -- )              drop drop  ; inl
: rrot  ( n1 n2 n3 -- n3 n1 n2 )   rot  rot  ; inl
: *     ( n1 n2 -- {n1*n2} )       du* drop  ; inl
: 1+    ( n1 -- {n1+1} )             1    +  ; inl
: 1-    ( n1 -- {n1+1} )            -1    +  ; inl
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
: IO_SYS_SCELL       FFFFFFF8 ; inl
: IO_SYS_DSTACK      FFFFFFF4 ; inl
: IO_SYS_RSTACK      FFFFFFF0 ; inl
: IO_SYS_SELECTOR    FFFFFFEC ; inl
: IO_SYS_VALUE       FFFFFFE8 ; inl
: SYS_TERMINATE             2 ; inl

public

internal current !
\ obtains the address of a system register
: sysreg ( n -- addr ) IO_SYS_SELECTOR ! IO_SYS_VALUE ;

\ terminates the program
: terminate ( status -- ) SYS_TERMINATE sysreg ! ;

\ obtains the address to access the data stack
: dstack ( idx -- addr ) IO_SYS_SCELL ! IO_SYS_DSTACK ;

\ obtains the address to access the return stack
: rstack ( idx -- addr ) IO_SYS_SCELL ! IO_SYS_RSTACK ;

forth current !

\ terminate the program successfully
: bye ( -- ) 0 terminate tail ; noexit

}scope

( *** deferred words *** )

\ emits one character to the standard output
align defer emit ( c -- )

\ types a counted string to the standard output
align defer type ( c-str n -- )

\ obtains an input character from standard input
align defer getc ( -- c)

\ prints an error message and restarts the interpreter
align defer error ( c-str n  -- )

\ prints an error message and quits
align defer fatal ( c-str n  -- )

( *** helper words *** )

\ returns true if min <= v <= max
: within ( v min max -- t )
  rot tuck u<
  if 2drop 0 exit then
  swap u>=
  ;

\ advances the counted string by a given number of characters
\ : /str ( c-str n num -- c-str' n' )
\  rot over + rrot -
\  ;

\ advances the counted string by 1
: 1/str ( c-str n -- c-str' n' )
  1- swap 1+ swap
  ;

\ copies a given string into the destination buffer
: str-copy ( c-str n dst -- )
  >r
  begin
    dup if
      over c@ r@ c!
      r> 1+ >r 1/str
      again
    then
  end
  2drop rdrop
  ;

\ finds a given character in the string
\ returns the index of the character
: char-find ( c-str c -- idx )
  over
  begin
    2dup c@ <>
    if 1+ again then
  end
  rot - nip
  ;

( *** words related to printing *** )
\ prints space
: space ( -- )  20 emit tail ; noexit

\ prints a newline to the console
: nl ( -- ) 0A emit tail ; noexit

( *** initializes the wordbuf *** )
scope{
ephemeral
( *** constants for the QUI vm *** )
: SYS_MEMSIZE               4 ; inl

private
\ initializes the wordbuf
: wordbuf_initialize ( -- )
  SYS_MEMSIZE sysreg @ [ wordbuf buf>end ] lit !
  ;
last @ >xt onboot !
}scope
