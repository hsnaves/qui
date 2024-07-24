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
: shl   ( n1 n2 -- {n1<<n2} )      [ CB c, ] ; inl
: ushr  ( u1 u2 -- {u1>>u2} )      [ CC c, ] ; inl
: shr   ( n1 n2 -- {n1>>n2} )      [ CD c, ] ; inl
: +     ( n1 n2 -- {n1+n2} )       [ CE c, ] ; inl
: -     ( n1 n2 -- {n1-n2} )       [ CF c, ] ; inl
: nop   ( -- )                     [ D0 c, ] ; inl
: dup   ( n -- n n )               [ D1 c, ] ; inl
: drop  ( n -- )                   [ D2 c, ] ; inl
: swap  ( n1 n2 -- n2 n1 )         [ D3 c, ] ; inl
: over  ( n1 n2 -- n1 n2 n1 )      [ D4 c, ] ; inl
: rot   ( n1 n2 n3 -- n2 n3 n1 )   [ D5 c, ] ; inl
: >r    ( n -- r: n )              [ D6 c, ] ; inl
: r>    ( r: n -- n )              [ D7 c, ] ; inl
: @     ( addr -- n )              [ D8 c, ] ; inl
: !     ( n addr -- )              [ D9 c, ] ; inl
: c@    ( addr -- c )              [ DA c, ] ; inl
: c!    ( c addr -- )              [ DB c, ] ; inl
: r@    ( raddr -- n )             [ DC c, ] ; inl
: r!    ( n raddr -- )             [ DD c, ] ; inl
: du*   ( u1 u2 -- lo high )       [ DE c, ] ; inl
: u/mod ( u1 u2 -- rem quot )      [ DF c, ] ; inl

( some other composite words )
: nip   ( n1 n2 -- n2 )           swap drop  ; inl
: tuck  ( n1 n2 -- n2 n1 n2 )     swap over  ; inl
: 2dup  ( n1 n2 -- n1 n2 n1 n2 )  over over  ; inl
: *     ( n1 n2 -- {n1*n2} )       du* drop  ; inl

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

: signe ( n bits -- n' )
  20 swap - dup >r shl r> shr
  ;

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
    2dup c@ = =0
    if 1 + again then
  end
  rot - nip
  ;

( *** deferred words *** )

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
