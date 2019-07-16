CFLAGS  = -g -Wall

cairo: cairo.c
	$(CC) $(CFLAGS) $^ -o $@ `pkg-config --cflags --libs cairo x11`

test_string_set: test_string_set.c string_set.c
	$(CC) $(CFLAGS) $^ -o $@
