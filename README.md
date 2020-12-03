# Lispy
*mini LISP interpreter*

created using http://www.buildyourownlisp.com/

### Commands
```bash
# compile
gcc -std=c99 -Wall parsing.c mpc.c -ledit -lm -o parsing.out

# run REPL
./parsing.out

# with memory check
gcc -std=c99 -Wall parsing.c mpc.c -ledit -lm -fsanitize=address -fno-common -fno-omit-frame-pointer -o parsing.out
```

### REPL example
```
Lispy Version 0.0.0.0.6
Press Ctrl+c to Exit

lispy> + 1 2 (/ 9 3)
6
lispy> + 3 (% 10 3)
4
lispy>
```

### TODO
* add tests
* extract some function to modules
* add makefile for running compilation, tests, memory checks
* run via lldb
