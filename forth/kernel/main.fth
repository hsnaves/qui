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

( *** words to change base *** )

\ change the base to hexadecimal
: hex ( -- ) 10 base c!  ;

\ change the base to decimal
: decimal ( -- ) 0A base c!  ;

( *** exec trampoline *** )

\ obtains the current value of the pc
: pc ( -- pc ) [ DF c, ] ;

\ executes a given function
: exec ( addr -- ... )
  pc 4 + -
  [ I_JMP c, ]
  ; noexit


( *** implementation of basic system words *** )

scope{
auxiliary
( *** constants for the QUI vm *** )
: IO_SYS_SCELL       FFFFFFF8 ; inl
: IO_SYS_DSTACK      FFFFFFF4 ; inl
: IO_SYS_RSTACK      FFFFFFF0 ; inl
: IO_SYS_STATUS      FFFFFFEC ; inl
: IO_SYS_TERMINATE   FFFFFFE8 ; inl
: IO_SYS_STACKSIZE   FFFFFFE4 ; inl
: IO_SYS_MEMSIZE     FFFFFFE0 ; inl
: IO_SYS_CELLSIZE    FFFFFFDC ; inl
: IO_SYS_ID          FFFFFFD8 ; inl
: IO_SYS_CYCLECOUNT  FFFFFFD4 ; inl
: CELL_STACK_POINTER FFFFFFFF ; inl

public
\ terminate the program
: terminate ( n -- ) IO_SYS_TERMINATE ! ;

\ halt the VM
: halt ( -- ) -1 IO_SYS_STATUS ! ;

\ terminate the program successfully
: bye ( -- ) 0 terminate tail ; noexit

\ obtains the address to access the data stack
: dstack ( idx -- addr ) IO_SYS_SCELL ! IO_SYS_DSTACK ;

\ obtains the address to access the return stack
: rstack ( idx -- addr ) IO_SYS_SCELL ! IO_SYS_RSTACK ;

\ Obtains the address to the data stack pointer
: dsp ( -- addr ) CELL_STACK_POINTER dstack tail ; noexit

\ Obtains the address to the return stack pointer
: rsp ( -- addr ) CELL_STACK_POINTER rstack tail ; noexit

\ Address of the status
: status ( -- addr ) IO_SYS_STATUS ; inl

\ obtains the memory size
: stacksize ( -- u ) IO_SYS_STACKSIZE @ ;

\ obtains the memory size
: memsize ( -- u ) IO_SYS_MEMSIZE @ ;

\ obtains the size of a cell
: cellsize ( -- u ) IO_SYS_CELLSIZE @ ;

\ obtains the ID of the VM
: sysid ( -- u ) IO_SYS_ID @ ;

\ obtains the number of cycles executed
: cyclecount ( -- u ) IO_SYS_CYCLECOUNT @ ;
}scope

( *** deferred words *** )

\ obtains an input character from standard input
align defer getc ( -- c)

\ emits one character to the standard output
align defer emit ( c -- )

\ types a counted string to the standard output
align defer type ( c-str n -- )

\ prints an error message and restarts the interpreter
align defer error ( c-str n  -- )

( *** memory allocation words *** )

\ shrinks wordbuf and returns the address of the
\ reserved memory following the shrunk buffer
\ returns zero when it fails
: alloc ( size -- addr )
  [ wordbuf buf>end ] lit @ tuck
  [ wordbuf buf>here ] lit @ -
  over u<
  if 2drop 0 exit then
  - dup [ wordbuf buf>end ] lit !
  ;


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
: space ( -- )  20 emit tail  ; noexit

\ prints a newline to the console
: nl ( -- ) 0A emit tail ; noexit

\ types a counted string to the standard output
: (type) ( c-str n -- )
  begin
    dup =0
    if 2drop exit then
    over c@ emit
    1/str
    again
  end
  ; noexit
' (type) is type

( *** initializes the wordbuf *** )
scope{
private
\ initializes the wordbuf
: wordbuf_initialize ( -- )
  memsize [ wordbuf buf>end ] lit !
  ;
last @ >xt onboot !

}scope
