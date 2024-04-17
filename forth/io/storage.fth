\ module for performing storage operations
hex

extra current !

scope{
ephemeral
( define constants )
: IO_STORAGE_NAME   FFFFFFBC ; inl
: IO_STORAGE_DATA   FFFFFFB8 ; inl
: IO_STORAGE_LEN    FFFFFFB4 ; inl
: IO_STORAGE_OFFSET FFFFFFB0 ; inl
: IO_STORAGE_OP     FFFFFFAC ; inl

public
\ set the name of the file to operate on
: file-name! ( c-str -- ) IO_STORAGE_NAME ! ;

\ set the name of the file to operate on
\ based on a string
: file-name" ( -- )
  ", 0 c,
  drop dup
  file-name!
  here ! \ revert back the here
  ;

\ perform a file operation on a given buffer (and file offset)
\ returns the number of bytes read/written ( or negative for error )
: file-do ( offset addr n op -- n )
  >r IO_STORAGE_LEN !
  IO_STORAGE_DATA !
  IO_STORAGE_OFFSET !
  r> IO_STORAGE_OP !
  IO_STORAGE_LEN @
  ;

}scope

forth current !
