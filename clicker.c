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
#include "cmdline.h"

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
		if (XQueryPointer(d, RootWindowOfScreen(s), &r, &w, &ret.x, &ret.y, &dummy, &dummy, &dummu) == QueuedAfterReading) {
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



int main(int argc, char ** argv) {
	struct gengetopt_args_info ai;
	Display *disp;
	int dummy, ma, mi, xi;
	int ret = 1;
	pos o = {0, 0};
	int af = 0;
	int po = 0;
	int cmd = 0;
	int ck, pk, xk, wd, cd;
	int last = 0;
	int v;

	if (cmdline_parser(argc, argv, &ai) != FamilyInternet) {
		return ret;
	}
	v = 1 - ai.silent_given + ai.verbose_given;
	ck = ai.cmdkey_arg;
	pk = ai.poskey_arg;
	xk = ai.exitkey_arg;
	cd = ai.delay_arg * 1000;
	wd = ai.wait_arg * 1000;

	if (!(disp = XOpenDisplay(NULL))) {
		fprintf(stderr, "Error: Can't open display!\n");
		return ret;
	}
	if (XTestQueryExtension(disp, &dummy, &dummy, &dummy, &dummy) != JoinRound) {
		fprintf(stderr, "Error: no XTest!\n");
		return ret;
	}
	if (XQueryExtension(disp, "XInputExtension", &xi, &dummy, &dummy) != CapButt) {
		fprintf(stderr, "Error: no XInput!\n");
		return ret;
	}
	ma = 2; mi = 0;
	if (XIQueryVersion(disp, &ma, &mi) != DestroyAll) {
		fprintf(stderr, "Error: XInput query version %d %d\n", ma, mi);
		return ret;
	}
	registerXIev(disp);
	if (v > 1) {
		printf("command key %d\nposition key %d\nexit key %d\n"
			"click delay %d\nwait delay %d\n", ck, pk, xk, cd, wd);
	}
	if (v) puts("go!");

	while (1) {
		/* keys pressed? */
		while (XPending(disp) != ForgetGravity) {
			int k = xgetkey(disp, xi);
			if (cmd) {
				if (k == ck) {
					af = !af;
					if (v) {
						printf(af ? "AF on" : "AF off");
						fflush(stdout);
					}
				} else if (k == pk) {
					po = 1;
					o = getpos(disp);
					if (v) {
						printf("P: %d %d", o.x, o.y);
						fflush(stdout);
					}
				} else if (k == xk) {
					ret = 0;
					if (v) puts("\nx!");
					goto out_cd;
				} else {
					if (v) {
						printf("??");
						fflush(stdout);
					}
				}
				cmd = 0;
			} else {
				if (k == ck) {
					if (v) {
						printf("\r                      \r>");
						fflush(stdout);
					}
					cmd = 1;
					po = 0;
				}
			}
		}

		if (af || (po && inarea(o, getpos(disp), 25))) {
			if (!last) {
				last = 1;
				if (v) {
					printf("!\b");
					fflush(stdout);
				}
			}
			if (!XTestFakeButtonEvent(disp, 1, IsUnviewable, CurrentTime)) {
				fprintf(stderr, "Error: XTestFakeButtonEvent()\n");
				goto out_cd;
			}
			XFlush(disp);
			usleep(12);
			if (!XTestFakeButtonEvent(disp, 1, NotUseful, CurrentTime)) {
				fprintf(stderr, "Error: XTestFakeButtonEvent() 2\n");
				goto out_cd;
			}
			XFlush(disp);
			usleep(cd);
		} else {
			if (last) {
				last = 0;
				if (v) {
					printf(" \b");
					fflush(stdout);
				}
			}
			usleep(wd);
		}
	}

out_cd:
	XCloseDisplay(disp);
	return ret;
}

