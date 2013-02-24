source_files = $(shell find ../src ../lib -type f -iname '*.c' | sed 's/^\.\.\/src\///')
obj_files = $(subst .c,.o,$(source_files))

LDFLAGS =
CFLAGS = -g

vpath %.c ../src/

all: etar_test

etar_test: $(obj_files)
	gcc $(LDFLAGS) $^ -o $@

%.o: %.c
	-mkdir -p $(dir $@)
	gcc $(CFLAGS) $< -c -o $@

.PHONY: all clean

clean:
	rm -rf $(obj_files) etar_test
