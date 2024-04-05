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
include" forth/kernel/parsing.fth"
include" forth/kernel/wordmain.fth"
include" forth/kernel/interp.fth"
include" forth/kernel/colon.fth"
include" forth/kernel/boot.fth"

\ end of the compilation

0 here @
meta-exit
forth current !
decimal

file-name" kernel.bin"
meta-buffer swap 2 file-do . nl
7 sysreg @ . nl \ print the cycles
bye
