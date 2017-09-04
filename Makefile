mona: Makefile mona.c
	gcc -DSHOWWINDOW -Wall -std=gnu99 -pedantic -O3 `pkg-config --cflags cairo x11 cairo-xlib` mona.c `pkg-config --libs cairo x11 cairo-xlib` -o mona
clean:
	rm -f mona

