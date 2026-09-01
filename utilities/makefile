CC       = cc # can also use clang, gcc, etc.
CC_FLAGS = -I./utilities

.PHONY: all
all: utilities

.PHONY: clean
clean:
	rm -rf dist/

.PHONY: utilities
utilities: dist/load dist/parse

dist/load: load.c helpers/base32.c helpers/serial.c
	mkdir -p dist/
	$(CC) $(CC_FLAGS) -o $@ $^

dist/parse: parse.c
	mkdir -p dist/
	$(CC) $(CC_FLAGS) -o $@ $^
