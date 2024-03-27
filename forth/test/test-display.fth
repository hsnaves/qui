
\ display device
hex
scope{

auxiliary
: IO_DISPLAY_MODE     FFFFFEBC ; inl
: IO_DISPLAY_WIDTH    FFFFFEB8 ; inl
: IO_DISPLAY_HEIGHT   FFFFFEB4 ; inl
: IO_DISPLAY_BUFFER   FFFFFEB0 ; inl
: IO_DISPLAY_STRIDE   FFFFFEAC ; inl
: IO_DISPLAY_WAITSYNC FFFFFEA8 ; inl

public

: display-setbuffer ( buffer stride -- )
   IO_DISPLAY_STRIDE !
   IO_DISPLAY_BUFFER !
   ;

: display-init ( mode width height -- )
   rot IO_DISPLAY_MODE !
   swap IO_DISPLAY_WIDTH !
   IO_DISPLAY_HEIGHT !
   ;

: display-waitsync ( -- )
   1 IO_DISPLAY_WAITSYNC !
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


