hex

\ build the meta dictionary first
include" forth/meta/basic.fth"
meta meta include" forth/bootstrap/globals.fth"
meta include" forth/bootstrap/comp.fth"
include" forth/bootstrap/flags.fth"
include" forth/bootstrap/wordmain.fth"
include" forth/bootstrap/find.fth"
include" forth/meta/interp.fth"
include" forth/bootstrap/other.fth"
include" forth/kernel/main.fth"
include" forth/meta/scope.fth"
include" forth/kernel/scopeimpl.fth"
include" forth/meta/init.fth"

\ start the meta-compiler

forth current !
meta use
meta-interpreter
