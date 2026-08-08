CC       = clang # can also use clang, gcc, etc.
CC_FLAGS = -I./utilities

.PHONY: all
all: load

.PHONY: clean
clean:
	rm -f load

.PHONY: utilities
utilities: load

load: load.c helpers/base32.c helpers/serial.c
	$(CC) $(CC_FLAGS) -o $@ $^
