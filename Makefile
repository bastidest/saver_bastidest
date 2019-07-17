CFLAGS  = -Wall -pedantic -Wextra -Wconversion
LDFLAGS = `pkg-config --cflags --libs cairo x11`

ifeq ($(mode),release)
   CFLAGS += -O3
else
   mode = debug
   CFLAGS += -g -DDEBUG
endif

all: cairo

cairo: cairo.c string_set.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

test: test_string_set
	valgrind ./$<

test_string_set: test_string_set.c string_set.o
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -vf *.o test_string_set cairo
