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

\ set the data pointer (and file offset) for the operation
: f-data! ( offset addr n -- )
  IO_STORAGE_LEN !
  IO_STORAGE_DATA !
  IO_STORAGE_OFFSET !
  ;

inner current !
\ obtains the configuration parameters
: f-push ( -- offset addr n c-str n' )
  IO_STORAGE_OFFSET @
  IO_STORAGE_DATA @
  IO_STORAGE_LEN @
  IO_STORAGE_NAME @
  IO_STORAGE_NAMELEN @
  ;
forth current !

\ perform a file operation
\ returns the number of bytes read/written (or negative for error)
: f-do ( op -- n )
  IO_STORAGE_OP !
  IO_STORAGE_LEN @
  ;

brief
\ terminates in brief for the forth/kernel/skip.fth
