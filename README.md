# dssoo-filesystem-sim
A file system driver implementation using C, containing the following main actions:

  - mkFS: creates a new file system.
  - mountFS/unmountFS: mounts or unmounts the desired file system.
  - createFile/removeFile: creates or removes the file with the specified name.
  - openFile/closeFile: opens or closes a file using a file name or a file descriptor.
  - readFile/writeFile: reads or writes the data provided in a buffer, the number of bytes specified.
  - lseekFile: sets the file pointer in a specific position of the file.
  - checkFS/checkFile: checks the integrity of the FS or file.
  - other actions...

