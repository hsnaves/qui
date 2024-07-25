hex

\ build the meta dictionary first
" forth/bootstrap/inline.fth" include
" forth/meta/basic.fth" include
meta meta " forth/bootstrap/globals.fth" include
meta " forth/bootstrap/comp.fth" include
" forth/bootstrap/wordmain.fth" include
" forth/bootstrap/find.fth" include
" forth/meta/interp.fth" include
" forth/bootstrap/other.fth" include
" forth/bootstrap/inline.fth" include
" forth/kernel/main.fth" include
" forth/meta/scope.fth" include
" forth/kernel/scopeimpl.fth" include
}scope
" forth/meta/init.fth" include

\ start the meta-compiler

forth current !
meta use
meta-interpreter
