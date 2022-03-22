#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif /* _XOPEN_SOURCE */

#include <sys/select.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <regex.h>
#include <ctype.h>
#include <locale.h>
#include <stdarg.h>

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>
//#include <X11/extensions/Xinerama.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

typedef struct {
	int x;
	int y;
} pos;

static pos getpos(Display *d) {
	int i, sc, dummy;
	unsigned dummu;
	pos ret;
	Window w = 0;
	Window r = 0;

	sc = ScreenCount(d);
	for (i = 0; i < sc ; i++) {
		Screen *s = ScreenOfDisplay(d, i);
		if (XQueryPointer(d, RootWindowOfScreen(s), &r, &w, &ret.x, &ret.y, &dummy, &dummy, &dummu) == True) {
			sc = i;
			break;
		}
	}
	return ret;
}

static int inarea(pos a, pos b, int d) {
	return ((a.x >= b.x - d) && (a.x <= b.x + d) && (a.y >= b.y - d) && (a.y <= b.y + d));
}



int main(void) {
	Display *disp;
	int dummy;
	int ret = 1;
	pos o, n;

	if (!(disp = XOpenDisplay(NULL))) {
		fprintf(stderr, "Error: Can't open display!\n");
		return ret;
	}
	if (XTestQueryExtension(disp, &dummy, &dummy, &dummy, &dummy) != True) {
		fprintf(stderr, "Error: no XTest!\n");
		return ret;
	}
	puts("sleeping...");
	sleep(1);
	puts("go!");
	o = getpos(disp);
	printf("position: %d %d\n", o.x, o.y);

	while (1) {
		/* FIXME: find condition to break out of loop */

		for (n = getpos(disp); inarea(o, n, 25); n = getpos(disp)) {

			if (!XTestFakeButtonEvent(disp, 1, True, CurrentTime)) {
				fprintf(stderr, "Error: XTestFakeButtonEvent()\n");
				goto out_cd;
			}
			XFlush(disp);
			usleep(12);
			if (!XTestFakeButtonEvent(disp, 1, False, CurrentTime)) {
				fprintf(stderr, "Error: XTestFakeButtonEvent() 2\n");
				goto out_cd;
			}
			XFlush(disp);
			usleep(25000);
		}
		printf("position: %d %d, waiting...\n", n.x, n.y);
		while (! inarea(o, getpos(disp), 25)) {
			usleep(25000);
		}
	}

	ret = 0;
out_cd:
	XCloseDisplay(disp);
	return ret;
}
