hex
include" forth/meta/meta.fth"

\ the bootstrap code
forth internal include" forth/bootstrap/globals.fth"
include" forth/bootstrap/main.fth"
internal current !
include" forth/io/console.fth"
include" forth/bootstrap/ibasic.fth"
forth current !
include" forth/bootstrap/herelast.fth"
internal current !
include" forth/bootstrap/imain.fth"
forth current !
include" forth/bootstrap/tib.fth"
include" forth/bootstrap/wordmain.fth"
include" forth/bootstrap/parsing.fth"
include" forth/bootstrap/interp.fth"
include" forth/bootstrap/other.fth"
include" forth/bootstrap/boot.fth"

\ end of the compilation

0 here @
meta-exit
forth current !
decimal

stg-name" bootstrap.rom"
meta-buffer swap 2 stg-do . nl
bye
