\ initialize the variables

hex

meta use

\ initialize global variables
forth current !
extra context !
0 this !
0 flags c!
0 state c!
0A base c!
01 janum c!

\ initialize the forth dictionary
internal forth node>next !
0 forth dict>last !
wordbuf forth dict>code !
wordbuf forth dict>data !
0 forth dict>index !

\ initialize the internal dictionary
0 internal node>next !
0 internal dict>last !
wordbuf internal dict>code !
wordbuf internal dict>data !
0 internal dict>index !

\ initialize the extra dictionary
forth extra node>next !
0 extra dict>last !
wordbuf extra dict>code !
wordbuf extra dict>data !
0 extra dict>index !

\ initialize the wordbuffer
\ use smaller size here
A8 wordbuf buf>here !
A8 wordbuf buf>start !
10000 wordbuf buf>end !
0 wordbuf buf>off !

\ initialize the temp dictionary
0 temp node>next !
0 temp dict>last !
tmpbuf temp dict>code !
tmpbuf temp dict>data !
0 temp dict>index !

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

meta-discard
