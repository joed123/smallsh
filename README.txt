Hello! 

Smallsh is a custom shell written in C, supporting command execution, I/O redirection, background/foreground processes, and built-in commands (cd, exit, status). It features $$ variable expansion and custom signal handling for SIGINT and SIGTSTP.

Heres how to compile this project:
gcc --std=gnu99 -o smallsh prog3.c

Then to run it:
./smallsh  
