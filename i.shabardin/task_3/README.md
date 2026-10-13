Build with `make`, then run `./setuid_test` from this directory.

To demonstrate the difference between real and effective UID on Solaris, run
`make prepare-demo`. This sets `data.txt` to mode 600 and the executable's
setuid bit. Ask another student to run the executable from this directory.
Their first file open should succeed as the owner; after `setuid(getuid())`,
the second open should fail. Git does not preserve these permission bits, so
repeat `make prepare-demo` after each checkout or rebuild.
