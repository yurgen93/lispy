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

.PHONY: build
build:
	@$(CC) $(CFLAGS) *.c -o lispy $(LIBS)

.PHONY: memcheck
memcheck:
	@echo Compiling $@
	@$(CC) $(ASANFLAGS) $(CFLAGS) *.c -o memcheck.out $(LIBS)
	@./memcheck.out examples/hello_world.lispy examples/def_vars.lispy examples/def_functions.lispy examples/list_manipulations.lispy
	@echo "Memory check passed"

.PHONY: clean
clean:
	rm -rf *.o *.out *.out.dSYM
