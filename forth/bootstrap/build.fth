hex
" forth/meta/meta.fth" include

\ the bootstrap code
forth inner %" forth/bootstrap/globals.fth" include
%" forth/bootstrap/main.fth" include
inner current !
%" forth/io/console.fth" include
forth %" forth/bootstrap/comp.fth" include
forth current !
%" forth/bootstrap/tib.fth" include
%" forth/bootstrap/wordmain.fth" include
%" forth/bootstrap/parsing.fth" include
%" forth/bootstrap/interp.fth" include
%" forth/bootstrap/other.fth" include
%" forth/bootstrap/boot.fth" include

\ end of the compilation

0 here @
meta-quit
forth current !
decimal

" bootstrap.rom" f-name!
meta-buffer swap 2 f-do . nl
bye
