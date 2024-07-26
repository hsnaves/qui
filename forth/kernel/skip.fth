
\ word to comipile invokation of another word
\ but skipping the pranble
\ the preamble is from the >xt to the first occurrence
\ of insn
: skip ( ins -- )
  word 1 lookup
  dup =0 if
    " ? " 0 error
    drop 2 error tail
  then
  nip nip >xt
  dup rot index + 1 +
  JSR j, tail
  ; noexit
