# bones-shell

A bare-bones shell implemented in C utilizing the concept of concurrency through forking.
Aims to illustrate the working principle of a shell using simple argument types (LIST, ECHO, FORK)

- LIST: list of arguments, which the shell processes recursively
- ECHO: single argument calling the `echo` program
- FORK: single argument calling any program implemented in the system running `bones-shell` - it is called fork because it runs the argument program concurrently in a forked child process
