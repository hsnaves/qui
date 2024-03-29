
\ display device
hex
scope{

auxiliary
: IO_DISPLAY_COMMAND  FFFFFEBC ; inl
: IO_DISPLAY_PARAM0   FFFFFEB8 ; inl
: IO_DISPLAY_PARAM1   FFFFFEB4 ; inl
: IO_DISPLAY_PARAM2   FFFFFEB0 ; inl
: IO_DISPLAY_PARAM3   FFFFFEAC ; inl
: IO_DISPLAY_PARAM4   FFFFFEA8 ; inl
: IO_DISPLAY_PARAM5   FFFFFEA4 ; inl
: IO_DISPLAY_PARAM6   FFFFFEA0 ; inl
: IO_DISPLAY_PARAM7   FFFFFE9C ; inl

: DISPLAY_CMD_INIT           1 ; inl
: DISPLAY_CMD_SETBUF         2 ; inl
: DISPLAY_CMD_WAITSYNC       3 ; inl
: DISPLAY_CMD_FRAMECOUNT     4 ; inl

public

: display-init ( mode width height -- )
   IO_DISPLAY_PARAM2 !
   IO_DISPLAY_PARAM1 !
   IO_DISPLAY_PARAM0 !
   DISPLAY_CMD_INIT IO_DISPLAY_COMMAND !
   ;

: display-setbuffer ( buffer stride -- )
   IO_DISPLAY_PARAM1 !
   IO_DISPLAY_PARAM0 !
   DISPLAY_CMD_SETBUF IO_DISPLAY_COMMAND !
   ;

: display-waitsync ( -- )
   1 IO_DISPLAY_PARAM0 !
   DISPLAY_CMD_WAITSYNC IO_DISPLAY_COMMAND !
   ;

}scope
decimal

\ keyboard device
hex
scope{
auxiliary
: IO_KEYBOARD_KEY0    FFFFFE3C ; inl
: IO_KEYBOARD_KEY1    FFFFFE38 ; inl
: IO_KEYBOARD_X       FFFFFE34 ; inl
: IO_KEYBOARD_Y       FFFFFE30 ; inl
: IO_KEYBOARD_BUTTON  FFFFFE2C ; inl

public

: keyboard-x ( -- x )
   IO_KEYBOARD_X @
   ;

: keyboard-y ( -- y )
   IO_KEYBOARD_Y @
   ;

: keyboard-button ( -- y )
   IO_KEYBOARD_BUTTON @
   ;

}scope
decimal

\ screen dimensions
: MODE 0 ; inl
: WIDTH 320 ; inl
: HEIGHT 200 ; inl

\ allocate the framebuffer
align WIDTH HEIGHT * var framebuffer

\ initialize the display
framebuffer WIDTH display-setbuffer
MODE WIDTH HEIGHT display-init

: render-loop
   begin
       keyboard-button
       dup 4 and if exit then
       1 and
       if
          255
          keyboard-y WIDTH *
          keyboard-x +
          framebuffer +
          c!
       then
       display-waitsync
       again
   end
   ;

render-loop
bye


