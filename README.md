# Lispy
*mini LISP interpreter*

created using http://www.buildyourownlisp.com/

### Commands
```bash
# compile
gcc -std=c99 -Wall lispy.c mpc.c -ledit -lm -o lispy

# run REPL
./lispy

# with memory check
gcc -std=c99 -Wall lispy.c mpc.c -ledit -lm -fsanitize=address -fno-common -fno-omit-frame-pointer -o lispy
```

### REPL example
```
Lispy Version 0.0.0.0.1
Press Ctrl+c to Exit

lispy> + 1 2 (/ 9 3)
6
lispy> + 3 (% 10 3)
4
lispy>
```
