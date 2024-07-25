\ initialize the variables

hex

meta use

\ initialize global variables
forth current !
forth context !
0 other !
0 this !
0 dfl c!
0 state c!
0A base c!
01 jsz c!

\ initialize the forth dictionary
inner forth dict>next !
0 forth dict>last !
wbuf forth dict>code !
wbuf forth dict>data !

\ initialize the inner dictionary
0 inner dict>next !
0 inner dict>last !
wbuf inner dict>code !
wbuf inner dict>data !

\ initialize the wbuf
\ use smaller size here
80 wbuf buf>here !
80 wbuf buf>start !
10000 wbuf buf>end !
0 wbuf buf>off !

\ initialize the temp dictionary
0 temp dict>next !
0 temp dict>last !
tmpbuf temp dict>code !
tmpbuf temp dict>data !

\ initialize the tmpbuf
\ make it smaller here to fit the current
\ wbuf
10000 tmpbuf buf>here !
10000 tmpbuf buf>start !
20000 tmpbuf buf>end !
0 tmpbuf buf>off !

\ initialize the tib
0 tib buf>here !
0 tib buf>start !
0 tib buf>end !
0 tib buf>off !

meta-scrap
