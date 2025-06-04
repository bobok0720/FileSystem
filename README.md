# FileSystem

This project provides a small in-memory file system.  The `fs` directory
contains both a local command line program and a TCP based server.

## Building

Run `make -C fs` to compile the binaries and `make test` to execute the unit
suite.

## Network usage

`./fs/FS` starts the file system server listening on port `666`.  The
`./fs/FC` utility connects to this server and sends commands.  Commands are the
same as those supported by `FS_local`.  A typical session looks like:

```sh
$ ./fs/FS &
$ ./fs/FC 666
login 1
f
mk foo 0b1111
ls
e
```

The client prints any responses from the server and exits when it receives
`Bye!`.
