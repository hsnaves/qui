hex

include" forth/meta/meta.fth"

\ the kernel code
include" forth/kernel/globals.fth"
include" forth/kernel/main.fth"
include" forth/io/console.fth"
internal current !
include" forth/kernel/ibasic.fth"
forth current !
include" forth/kernel/herelast.fth"
internal current !
include" forth/kernel/imain.fth"
forth current !
include" forth/kernel/parsing.fth"
internal current !
include" forth/kernel/iextra.fth"
forth current !
include" forth/kernel/wordmain.fth"
include" forth/kernel/interp.fth"
include" forth/kernel/colon.fth"
include" forth/kernel/boot.fth"

\ end of the compilation

here @
meta-exit
forth current !
decimal

meta-buffer swap file-buffer!
0 file-offset!
file-name" kernel.bin"
file-write . nl
cyclecount . nl
bye
