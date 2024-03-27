hex

\ build the meta dictionary first
include" forth/meta/basic.fth"
include" forth/kernel/globals.fth"
include" forth/kernel/ibasic.fth"
include" forth/kernel/herelast.fth"
include" forth/kernel/imain.fth"
include" forth/kernel/iextra.fth"
include" forth/kernel/wordmain.fth"
include" forth/meta/interp.fth"
include" forth/kernel/colon.fth"
include" forth/rom/main.fth"
include" forth/meta/scope.fth"
include" forth/rom/scopeimpl.fth"

\ start the meta-compiler

forth current !
meta use

\ initialize global variables
forth current !
forth context !
0 this !
0 flags c!
0 state c!
0A base c!
01 janum c!

\ initialize the forth dictionary
0 forth dict>last !
internal forth dict>next !
wordbuf forth dict>code !
wordbuf forth dict>data !

\ initialize the internal dictionary
0 internal dict>last !
0 internal dict>next !
wordbuf internal dict>code !
wordbuf internal dict>data !

\ initialize the wordbuffer
\ use smaller size here
88 wordbuf buf>here !
88 wordbuf buf>start !
10000 wordbuf buf>end !
0 wordbuf buf>off !

\ initialize the temp dictionary
0 temp dict>last !
0 temp dict>next !
tmpbuf temp dict>code !
tmpbuf temp dict>data !

\ initialize the tmpbuffer
\ make it smaller here to fit the current
\ wordbuf
10000 tmpbuf buf>here !
10000 tmpbuf buf>start !
20000 tmpbuf buf>end !
0 tmpbuf buf>off !

\ initialize the tib
0 tib buf>here !
0 tib buf>start !
0 tib buf>end !
0 tib buf>off !

interpreter
