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
brief
( *** constants for the QUI vm *** )
: IO_SYS_SELECTOR    FFFFFFF8 ; inl
: IO_SYS_VALUE       FFFFFFF4 ; inl
: SYS_STATUS                1 ; inl
: SYS_DSP                   2 ; inl
: SYS_RSP                   3 ; inl
: SYS_ISTART                8 ; inl
: SYS_IEND                  9 ; inl

public

inner current !
\ obtains the address of a system register
: sreg ( n -- addr ) IO_SYS_SELECTOR ! IO_SYS_VALUE ;

\ set status value
: sts! ( v -- ) SYS_STATUS sreg ! ;

\ the address of the data stack pointer
: dsp ( -- addr ) SYS_DSP sreg tail ; noexit

\ the address of the return stack pointer
: rsp ( -- addr ) SYS_RSP sreg tail ; noexit

\ flushes the code cache
: flush ( iend istart -- )
  SYS_ISTART sreg !
  SYS_IEND sreg !
  ;

forth current !

\ terminate the program successfully
: bye ( -- ) 0 sts! tail ; noexit

}scope

( *** helper words *** )

\ advances the counted string by 1
: /s ( c-str n -- c-str' n' )
  1 - swap 1 + swap
  ;

\ copies a given string into the destination buffer
: copy ( c-str n dst -- )
  begin
    over if
      >r over c@ 0 r@ c!
      /s r> 1 +
      again
    then
  end
  drop drop drop
  ;

\ finds the index of given character in the string
\ returns the index of the character
: index ( c-str c -- idx )
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
    dup if
      over c@ emit /s
      again
    then
  end
  drop drop
  ;

( *** words related to printing *** )
\ prints space
: space ( -- )  20 emit tail ; noexit

\ prints a newline to the console
: nl ( -- ) 0A emit tail ; noexit

( *** initializes the wbuf *** )
scope{
brief
( *** constants for the QUI vm *** )
: SYS_MEMSIZE               5 ; inl

private
\ initializes the wbuf
: wbuf_initialize ( -- )
  SYS_MEMSIZE sreg @ [ wbuf buf>end ] lit !
  ;
last @ >xt onboot !
}scope
