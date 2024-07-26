\ module for performing storage operations
hex

brief
( define constants )
: IO_STORAGE_NAME    FFFFFFBC ; inl
: IO_STORAGE_NAMELEN FFFFFFB8 ; inl
: IO_STORAGE_DATA    FFFFFFB4 ; inl
: IO_STORAGE_LEN     FFFFFFB0 ; inl
: IO_STORAGE_OFFSET  FFFFFFAC ; inl
: IO_STORAGE_OP      FFFFFFA8 ; inl

public
\ set the name of the file to operate on
: f-name! ( c-str n -- )
  IO_STORAGE_NAMELEN !
  IO_STORAGE_NAME !
  ;

\ perform a file operation on a given buffer (and file offset)
\ returns the number of bytes read/written ( or negative for error )
: f-do ( offset addr n op -- n )
  >r IO_STORAGE_LEN !
  IO_STORAGE_DATA !
  IO_STORAGE_OFFSET !
  r> IO_STORAGE_OP !
  IO_STORAGE_LEN @
  ;

brief
\ terminates in brief for the forth/kernel/skip.fth
