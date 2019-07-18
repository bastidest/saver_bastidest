CFLAGS  = -Wall -pedantic -Wextra -Wconversion
LDFLAGS = `pkg-config --cflags --libs cairo x11`

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

ifeq ($(mode),release)
   CFLAGS += -O3
else
   mode = debug
   CFLAGS += -g -DDEBUG
endif

all: saver_bastidest

saver_bastidest: saver_bastidest.c string_set.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

test: test_string_set
	valgrind ./$<

test_string_set: test_string_set.c string_set.o
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -vf *.o test_string_set saver_bastidest

install: saver_bastidest
	install -d $(DESTDIR)$(PREFIX)/bin/saver_bastidest/
	install -m 755 saver_bastidest $(DESTDIR)$(PREFIX)/bin/saver_bastidest/
	install -m 755 saver_bastidest_random $(DESTDIR)$(PREFIX)/bin/saver_bastidest/
