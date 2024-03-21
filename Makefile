
# vim: ft=make noexpandtab

FLOPPCGI_SOURCES := src/floppcgi.c src/parse.c

CFLAGS := -DDEBUG -Wall -g

LDFLAGS := -g

.PHONY: clean

all: floppcgi

floppcgi: $(addprefix obj/,$(subst .c,.o,$(FLOPPCGI_SOURCES)))
	$(CC) -o $@ $^ $(LDFLAGS) -lfcgi

obj/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $< 

clean:
	rm -rf floppcgi obj

