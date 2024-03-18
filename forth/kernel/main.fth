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
: sge8  ( c -- n )                 [ D4 c, ] ; inl
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
: hex ( -- )
   10 base c!
   ;

\ change the base to decimal
: decimal ( -- )
   0A base c!
   ;

( *** exec trampoline *** )

\ obtains the current value of the pc
: pc ( -- pc )
   [ DF c, ]                    \ d: pc | r: pc
   ;

\ executes a given function
: exec ( addr -- ... )
   pc                           \ d: addr pc
   4 + -                        \ d: rel_addr
   [ I_JMP c, ]                 \ compile jump
   ; noexit


( *** implementation of basic system words *** )

scope{
auxiliary

( *** constants for the QUI vm *** )
: IO_SYS_SCELL       FFFFFFF8 ; inl
: IO_SYS_DSTACK      FFFFFFF4 ; inl
: IO_SYS_RSTACK      FFFFFFF0 ; inl
: IO_SYS_STACKSIZE   FFFFFFE4 ; inl
: IO_SYS_MEMSIZE     FFFFFFE0 ; inl
: CELL_STACK_POINTER FFFFFFFF ; inl

public

\ Obtains the address to the data stack pointer
: dsp ( -- addr )
   CELL_STACK_POINTER
   IO_SYS_SCELL !               \ set the SCELL to -1
   IO_SYS_DSTACK                \ return the address of DSTACK
   ;

\ Obtains the address to the return stack pointer
: rsp ( -- addr )
   CELL_STACK_POINTER
   IO_SYS_SCELL !               \ set the SCELL to -1
   IO_SYS_RSTACK                \ return the address of DSTACK
   ;

\ obtains the memory size
: stacksize ( -- u )
   IO_SYS_STACKSIZE @
   ;

\ obtains the memory size
: memsize ( -- u )
   IO_SYS_MEMSIZE @
   ;

}scope

( *** deferred words *** )

\ word to be called when there is an error
align defer onerror ( c-str n -- )

\ word to be called when an exception is raised
align defer onexception ( status -- )

\ word to be called when the system boots
align defer onboot ( -- )

\ obtains an input character from standard input
align defer getc ( -- c)

\ emits one character to the standard output
align defer emit ( c -- )

\ types a counted string to the standard output
align defer type ( c-str n -- )

( *** memory allocation words *** )

\ shrinks wordbuf and returns the address of the
\ reserved memory following the shrunk buffer
\ returns zero when it fails
: alloc ( size -- addr )
   [ wordbuf buf>end ] lit @    \ d: size wb_end
   tuck                         \ d: wb_end size wb_end
   [ wordbuf buf>here ] lit @   \ d: wb_end size wb_end wb_here
   -                            \ d: wb_end size wb_size
   over u<                      \ d: wb_end size large?
   if 2drop 0 then              \ return 0
   -                            \ d: addr
   dup                          \ d: addr addr
   [ wordbuf buf>end ] lit !    \ d: addr
   ;


( *** helper words *** )

\ sign-extend n-bit value
: sge ( v bits -- v' )
   dup 20 u>=                   \ d: v bits overflow?
   if drop exit then
   20 swap -                    \ d: v bits'
   tuck                         \ d: bits v bits
   shl swap shr                 \ d: v
   ;

\ returns true if min <= v <= max
: within ( v min max -- t )
   rot tuck u<                  \ d: min v greater
   if 2drop 0 exit then         \ check if (v > max)
   swap u>=                     \ return (v >= min)
   ;

\ advances the counted string by one character
: str1+ ( c-str n -- {c-str+1} {n-1} )
   1- swap 1+ swap              \ d: c-str' n'
   ;

\ copies a given string into the destination buffer
: str-copy ( c-str n dst -- )
   >r                           \ d: c-str n | r: dst
   begin
      dup if                    \ d: c-str n | r: dst
         over c@                \ d: c-str n c | r: dst
         r@ c!                  \ d: c-str n | r: dst
         r> 1+ >r               \ d: c-str n | r: dst'
         str1+                  \ d: c-str' n' | r: dst'
         again
      then
   end
   2drop                        \ d: | r: dst
   rdrop                        \ d:
   ;

\ finds a given character in the string
\ returns the index of the character
: char-find ( c-str c -- idx )
   over                         \ d: orig c c-str
   begin
      2dup                      \ d: orig c c-str c c-str
      c@ <>                     \ d: orig c c-str new?
      if 1+ again then          \ d: orig c c-str'
   end
   rot -                        \ d: c idx
   nip                          \ d: idx
   ;

( *** words related to printing *** )

\ prints space
: space
   20 emit tail
   ; noexit

\ prints a newline to the console
: nl ( -- )
   0A emit tail
   ; noexit

\ types a counted string to the standard output
: (type) ( c-str n -- )
   begin
      dup                       \ d: c-str n n
      =0 if                     \ d: c-str n
         2drop exit             \ terminate
      then
      over c@                   \ d: c-str n c
      emit                      \ d: c-str n
      str1+                     \ d: c-str' n'
      again
   end
   ; noexit
' (type) is type

\ prints an error message of an unknown word
," ? "                          \ d: c-str n
: unknown ( c-str n -- )
   [ swap ] lit lit             \ d: c-str n c-str' n'
   1 channel c!                 \ set the channel to stderr
   type
   0 channel c!                 \ revert back to stdout
   onerror tail
   ; noexit

( *** initializes the wordbuf *** )

scope{
private

\ initializes the wordbuf
: wordbuf_initialize ( -- )
   defer-chain onboot
   memsize                      \ d: vend
   [ wordbuf buf>end ] lit !    \ d:
   ;

}scope
