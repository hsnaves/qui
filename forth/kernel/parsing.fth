\ implementation of parsing words

hex

( *** words related to the TIB *** )

scope{
public

\ memory allocation function with validation
\ if it returns, the address is guarateed to be non-zero
\ otherwise it terminates the program
," memory exhausted"
: allocate ( size -- addr )
   alloc                        \ d: addr
   dup if exit then             \ d: addr
   drop                         \ d:
   1 channel !                  \ set channel to stderr
   [ swap ] lit lit             \ embed the string
   type nl                      \ d:
   1 terminate tail
   ; noexit

auxiliary

\ size of the tib
: TIB_SIZE 1000 ; inl

private

\ initializes the tib
: tib_initialize ( -- )
   [ onboot @ ] lit exec
   TIB_SIZE allocate            \ d: addr
   dup [ tib buf>here ] lit !   \ d: addr
   dup [ tib buf>start ] lit !  \ d: addr
   dup [ tib buf>off ] lit !    \ d: addr
   TIB_SIZE +                   \ d: vend
   [ tib buf>end ] lit !        \ d:
   ;
last @ >xt onboot !

}scope


scope{
auxiliary

\ checks for a newline
: nl? ( c -- b )
   0A =
   ; inl

private

\ prints an error messages that the TIB overflowed
," TIB overflow"
: tiboverflow ( -- )
   [ swap ] lit lit             \ d: c-str n
   error tail
   ; noexit

public

\ Obtains one line from the input
: line ( -- )
   [ tib buf>start ] lit        \ d: start
   @ dup                        \ d: vstart vstart
   [ tib buf>here ] lit !       \ d: vstart
   [ tib buf>off ] lit !        \ d:
   begin                        \ d:
      getc                      \ d: c
      [ tib buf>here ] lit @    \ d: c vhere
      2dup c!                   \ d: c vhere
      1+                        \ d: c vhere'
      [ tib buf>here ] lit !    \ d: c
      nl? =0                    \ d: notnl?
      if
         [ tib buf>here ] lit @ \ d: vhere
         [ tib buf>end ] lit @  \ d: vhere vend
         u< if again then
         tiboverflow
      then
   end
   ;
}scope


scope{
private

\ ensures that the TIB has some character
: ensuretib ( -- )
   begin
      [ tib buf>off ] lit @     \ d: voffset
      [ tib buf>here ] lit @    \ d: voffset vhere
      u>=                       \ d: empty?
      if line again then
   end
   ;

public

\ Obtains a character from the TIB
: key ( -- c )
   ensuretib
   [ tib buf>off ] lit          \ d: offset
   dup @                        \ d: offset voffset
   tuck 1+                      \ d: voffset offset voffset'
   swap !                       \ d: voffset
   c@                           \ d: c
   ;

}scope

scope{
auxiliary

\ checks if a character is blank
: blank? ( c -- b )
   \ all characters before ! are blanks
   [ char ! ] lit u<
   ; inl

public

\ Obtains a word from the TIB
: word ( -- c-str n )
   begin
      key blank?                \ d: blank?
      if again then
   end
   [ tib buf>off ] lit          \ d: offset
   @ dup 1- swap                \ d: c-str voffset
   [ tib buf>here ] lit @       \ d: c-str voffset vhere
   begin
      2dup u<                   \ d: c-str voffset vhere rem?
      if                        \ d: c-str voffset vhere
         over c@ blank? =0      \ d: c-str voffset vhere nonblank?
         if
            swap 1+ swap again
         then
      then
   end                          \ d: c-str voffset vhere
   drop dup 1+                  \ d: c-str voffset voffset'
   [ tib buf>off ] lit !        \ d: c-str voffset
   over -                       \ d: c-str n
   ;

}scope


( *** implementation of the compare word *** )

\ compare two counted strings
\ returns true when not equal
: compare ( c-str1 n1 c-str2 n2 -- neq? )
   swap >r                      \ d: c-str1 n1 n2 | r: c-str2
   over <>                      \ d: c-str1 n1 neq? | r: c-str2
   if
      2drop r>                  \ d: c-str2
      drop 1 exit               \ d: 1
   then

   begin
      dup if                    \ d: c-str1 n1 | r: c-str2
         over c@ r@ c@ =        \ d: c-str1 n1 c1 eq? | r: c-str2
         if
            1 /str              \ d: c-str1' n1' | r: c-str2
            r> 1+ >r            \ d: c-str1 n1 | r: c-str2'
            again
         then
      then
   end
   nip rdrop                    \ d: n1
   ;


( *** implementation of the number word *** )

scope{
private

\ character to digit word
: c>d ( c -- dig )
   dup                          \ d: c c
   [ char 9 char 0 ] lit lit    \ d: c c '0' '9'
   within                       \ d: c digit?
   if
      [ char 0 ] lit -
      exit                      \ return c - '0'
   then
   dup                          \ d: c c
   [ char Z char A ] lit lit    \ d: c c 'A' 'Z'
   within                       \ d: c letter?
   =0 if drop -1 exit then      \ return -1
   [ char A 0A - ] lit -        \ return c - 'A' + 10
   ;

\ counted string to unsigned number
\ returns the parsed number together with the number of
\ remaining characters in the counted string
: unumber ( c-str n -- u rem )
   swap 0                       \ d: n c-str 0
   begin                        \ d: n c-str u
      over c@                   \ d: n c-str u c
      c>d                       \ d: n c-str u dig
      base c@                   \ d: n c-str u dig vbase
      2dup u<                   \ d: n c-str u dig vbase okay
      if                        \ d: n c-str u dig vbase
         rot * +                \ d: n c-str u
         rot 1-                 \ d: c-str u n-1
         >r r@ rot              \ d: u n c-str | r: n
         1+ rot r>              \ d: n c-str' u n
         =0 until               \ d: n c-str u
         2dup                   \ d: n c-str u bogus bogus
      then                      \ d: n c-str u dig vbase
      2drop                     \ d: n c-str u
   end                          \ d: n c-str u
   nip swap                     \ d: u n
   ;

public

\ counted string to signed number
\ returns the parsed number and the number of remaining
\ characters in the counted string
: number ( c-str n -- num rem )
   1 over u<                    \ d: c-str n big_len?
   if                           \ d: c-str n
      over c@                   \ d: c-str n c
      [ char - ] lit  =         \ d: c-str n is_eq?
      if                        \ d: c-str n
         1 /str                 \ d: c-str' n'
         unumber                \ d: num rem
         0 rot                  \ d: rem 0 num
         - swap                 \ d: -num rem
         exit                   \ return
      then
   then
   unumber tail
   ; noexit

}scope

\ prints an error message of an unknown word
," ? "
: unknown ( c-str n -- )
   [ swap ] lit lit             \ compile the string
   1 channel !                  \ set the channel to stderr
   type                         \ d: c-str n
   error tail                   \ d:
   ; noexit

