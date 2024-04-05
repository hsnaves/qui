hex

\ build the meta dictionary first
include" forth/meta/basic.fth"
meta meta include" forth/kernel/globals.fth"
include" forth/kernel/ibasic.fth"
include" forth/kernel/herelast.fth"
include" forth/kernel/imain.fth"
include" forth/kernel/wordmain.fth"
include" forth/meta/interp.fth"
include" forth/kernel/colon.fth"
include" forth/rom/main.fth"
include" forth/meta/scope.fth"
include" forth/rom/scopeimpl.fth"
include" forth/meta/init.fth"

\ start the meta-compiler

forth current !
meta use
interpreter
