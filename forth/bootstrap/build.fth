hex
include" forth/meta/meta.fth"

\ the bootstrap code
forth inner include" forth/bootstrap/globals.fth"
include" forth/bootstrap/main.fth"
inner current !
include" forth/io/console.fth"
forth include" forth/bootstrap/comp.fth"
forth current !
include" forth/bootstrap/tib.fth"
include" forth/bootstrap/wordmain.fth"
include" forth/bootstrap/parsing.fth"
include" forth/bootstrap/interp.fth"
include" forth/bootstrap/other.fth"
include" forth/bootstrap/boot.fth"

\ end of the compilation

0 here @
meta-quit
forth current !
decimal

f-name" bootstrap.rom"
meta-buffer swap 2 f-do . nl
bye
