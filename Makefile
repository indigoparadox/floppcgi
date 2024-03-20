
# vim: ft=make noexpandtab

FLOPPCGI_SOURCES := src/floppcgi.c

.PHONY: clean

all: floppcgi

floppcgi: $(addprefix obj/,$(subst .c,.o,$(FLOPPCGI_SOURCES)))
	$(CC) -o $@ $^ -lfcgi

obj/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) -c -o $@ $< 

clean:
	rm -rf floppcgi obj

