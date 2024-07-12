hex
include" forth/meta/meta.fth"

\ the kernel code
forth internal include" forth/kernel/globals.fth"
include" forth/kernel/main.fth"
internal current !
include" forth/io/console.fth"
include" forth/kernel/ibasic.fth"
forth current !
include" forth/kernel/herelast.fth"
internal current !
include" forth/kernel/imain.fth"
forth current !
include" forth/kernel/tib.fth"
include" forth/kernel/wordmain.fth"
include" forth/kernel/parsing.fth"
include" forth/kernel/interp.fth"
include" forth/kernel/other.fth"
include" forth/kernel/boot.fth"

\ end of the compilation

0 here @
meta-exit
forth current !
decimal

stg-name" kernel.rom"
meta-buffer swap 2 stg-do . nl
bye
