\ module for performing storage operations

hex

scope{
auxiliary

( define constants )
: IO_STORAGE_NAME   FFFFFF7C ; inl
: IO_STORAGE_DATA   FFFFFF78 ; inl
: IO_STORAGE_LEN    FFFFFF74 ; inl
: IO_STORAGE_OFFSET FFFFFF70 ; inl
: IO_STORAGE_OP     FFFFFF6C ; inl

: STORAGE_OP_READ          1 ; inl
: STORAGE_OP_WRITE         2 ; inl

private

\ perform the operation
: do-operation ( op -- n )
   IO_STORAGE_OP !
   IO_STORAGE_LEN @
   ;

public

\ set the name of the file to operate on
: file-setname ( c-str n -- )
   drop
   IO_STORAGE_NAME !
   ;

\ set the offset for reading (or for appending when writing)
: file-setoffset ( offset -- )
   IO_STORAGE_OFFSET !
   ;

\ set the buffer to be used for the file operations
: file-setbuffer ( addr n -- )
   IO_STORAGE_LEN !
   IO_STORAGE_DATA !
   ;

\ write the file on the provided buffer
\ returns the number of bytes read
: file-read ( -- n )
   STORAGE_OP_READ do-operation tail
   ; noexit

\ write the file using the data provided in the buffer
\ returns the number of bytes written
: file-write ( -- n )
   STORAGE_OP_WRITE do-operation tail
   ; noexit

}scope

decimal
