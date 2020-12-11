LIBS = -lm
LIBS += -ledit

###
CFLAGS  = -std=c99
CFLAGS += -g
CFLAGS += -Wall
CFLAGS += -Wmissing-declarations
CFLAGS += -DUNITY_SUPPORT_64

ASANFLAGS  = -fsanitize=address
ASANFLAGS += -fno-common
ASANFLAGS += -fno-omit-frame-pointer

LISPY_EXAMPLES  = examples/hello_world.lispy
LISPY_EXAMPLES += examples/def_vars.lispy
LISPY_EXAMPLES += examples/def_functions.lispy
LISPY_EXAMPLES += examples/list_manipulations.lispy

.PHONY: build
build:
	@$(CC) $(CFLAGS) *.c -o lispy $(LIBS)

.PHONY: memcheck
memcheck:
	@echo Compiling $@
	@$(CC) $(ASANFLAGS) $(CFLAGS) *.c -o memcheck.out $(LIBS)
	@./memcheck.out $(LISPY_EXAMPLES)
	@echo "Memory check passed"

.PHONY: clean
clean:
	rm -rf *.o *.out *.out.dSYM

.PHONY: check_leaks
check_leaks:
	@echo Compiling $@
	@$(CC) $(CFLAGS) *.c -o lispy $(LIBS)
	@leaks -atExit  -- ./lispy $(LISPY_EXAMPLES)
	@echo "Memory leaks check passed"
