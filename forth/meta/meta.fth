hex

\ build the meta dictionary first
include" forth/meta/basic.fth"
include" forth/kernel/globals.fth"
include" forth/kernel/compbasic.fth"
include" forth/kernel/herelast.fth"
include" forth/kernel/compmain.fth"
include" forth/kernel/compextra.fth"
include" forth/kernel/wordmain.fth"
include" forth/meta/interp.fth"
include" forth/kernel/colon.fth"
include" forth/rom/control.fth"
include" forth/meta/scope.fth"
include" forth/rom/scopeimpl.fth"

\ start the meta-compiler

forth current !
meta use

\ initialize global variables
forth current !
compiler context !
00 this !
00 flags c!
00 state c!
0A base c!

\ initialize the forth dictionary
00 forth dict>last !
00 forth dict>next !
wordbuf forth dict>code !
wordbuf forth dict>data !

\ initialize the compiler dictionary
00 compiler dict>last !
forth compiler dict>next !
wordbuf compiler dict>code !
wordbuf compiler dict>data !

\ initialize the wordbuffer
\ use smaller size here
88 wordbuf buf>here !
88 wordbuf buf>start !
10000 wordbuf buf>end !
100 wordbuf buf>off !

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
10000 tmpbuf buf>off !

\ initialize the tib
0 tib buf>here !
0 tib buf>start !
0 tib buf>end !
0 tib buf>off !

interpreter
