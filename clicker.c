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
#include <X11/extensions/XInput2.h>
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

static void registerXIev(Display *disp) {
	Window root = DefaultRootWindow(disp);
	XIEventMask m;
	m.deviceid = XIAllMasterDevices;
	m.mask_len = XIMaskLen(XI_LASTEVENT);
	m.mask = calloc(m.mask_len, 1);
	XISetMask(m.mask, XI_RawKeyPress);
	XISelectEvents(disp, root, &m, 1);
	XSync(disp, False);
	free(m.mask);
}

static int xgetkey(Display * disp, int xi) {
	XEvent ev;
	XGenericEventCookie *c = (XGenericEventCookie*)&ev.xcookie;
	int ret;
	XNextEvent(disp, &ev);
	if (XGetEventData(disp, c)) {
		if ((c->type == GenericEvent) && (c->extension == xi) && (c->evtype == XI_RawKeyPress)) {
			XIRawEvent *r = c->data;
			ret = r->detail;
		} else {
			fprintf(stderr, "Error: XGetEventData returned strage stuff\n");
			exit(1);
		}
	} else {
		fprintf(stderr, "Error: XGetEventData is strange\n");
		exit(1);
	}
	return ret;
}



int main(void) {
	Display *disp;
	int dummy, ma, mi, xi;
	int ret = 1;
	pos o = {0, 0};
	int af = 0;
	int po = 0;
	int cmd = 0;
	int ck;

		if (!(disp = XOpenDisplay(NULL))) {
		fprintf(stderr, "Error: Can't open display!\n");
		return ret;
	}
	if (XTestQueryExtension(disp, &dummy, &dummy, &dummy, &dummy) != True) {
		fprintf(stderr, "Error: no XTest!\n");
		return ret;
	}
	if (XQueryExtension(disp, "XInputExtension", &xi, &dummy, &dummy) != True) {
		fprintf(stderr, "Error: no XInput!\n");
		return ret;
	}
	ma = 2; mi = 0;
	if (XIQueryVersion(disp, &ma, &mi) != Success) {
		fprintf(stderr, "Error: XInput query version %d %d\n", ma, mi);
		return ret;
	}
	registerXIev(disp);
	puts("press command key");
	ck = xgetkey(disp, xi);
	printf("command key %d\n", ck);
	puts("go!");

	while (1) {
		/* keys pressed? */
		while (XPending(disp)) {
			int k = xgetkey(disp, xi);
			if (cmd) {
				if (k == ck) {
					af = !af;
					printf("AF: %d\n", af);
				} else if (k == 38 /* a */) {
					po = 1;
					o = getpos(disp);
					printf("P: %d %d\n", o.x, o.y);
				} else if (k == 53 /* x */) {
					break;
				} else {
					puts("??");
				}
				cmd = 0;
			} else {
				if (k == ck) {
					putc('?', stdout);
					fflush(stdout);
					cmd = 1;
					po = 0;
				}
			}
		}

		if (af || (po && inarea(o, getpos(disp), 25))) {
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
		}
		usleep(25000);
	}

	ret = 0;
out_cd:
	XCloseDisplay(disp);
	return ret;
}

