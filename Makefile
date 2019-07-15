cairo: cairo.c
	gcc -Wall cairo.c -o cairo `pkg-config --cflags --libs cairo x11`

x11: main.c
	gcc -Wall main.c -o main `pkg-config --cflags --libs x11`
