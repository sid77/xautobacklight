/*
 * Original code Copyright (c) 2016 Ted Unangst <tedu@openbsd.org>,
 * released under public domain
 * Copyright (c) 2016 Marco Bonetti <sid77@slackware.it>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <X11/extensions/Xrandr.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>

pid_t
setbrightness(int percent)
{
        char *argv[] = { "xbacklight", "-set", NULL, NULL };
        static int percent_s_size = 3;
        char percent_s[percent_s_size];
        pid_t pid;

        snprintf(percent_s, percent_s_size, "%d", percent);
        argv[2] = percent_s;

        switch(pid = fork()) {
                case -1:
                        warn("cannot fork");
                        exit -1;
                case 0:
                        execvp("xbacklight", argv);
                        break;
                default:
                        return(pid);
        }
}

double
getbrightness(Display *dpy, int screen, Window root)
{
	double brightness = 0;
	XImage *image;
	int h = DisplayHeight(dpy, screen);
	int w = DisplayWidth(dpy, screen);
	int size;
	
	image = XGetImage(dpy, root, 0, 0, w, h, AllPlanes, ZPixmap);
	size = image->bytes_per_line * image->height;
	for (int i = 0; i < h; i++) {
		unsigned char *line = image->data + i * image->bytes_per_line;
		for (int j = 0; j < w; j++) {
			unsigned char *pix = line + j * image->bits_per_pixel / 8;
			unsigned char r = pix[0];
			unsigned char g = pix[1];
			unsigned char b = pix[2];
			unsigned char a = pix[3];

			brightness += r + b * 1.2 + g * 1.5;
		}
	}
	XDestroyImage(image);

	return brightness / (w * h);
}

void
adjustgamma(Display *dpy, Window root, double cutoff)
{
	XRRScreenResources *res = XRRGetScreenResourcesCurrent(dpy, root);

	int num_crtcs = res->ncrtc;
	for (int c = 0; c < res->ncrtc; c++) {
		int crtcxid = res->crtcs[c];
		XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(dpy, res, crtcxid);
		int size = XRRGetCrtcGammaSize(dpy, crtcxid);
		XRRCrtcGamma *crtc_gamma = XRRAllocGamma(size);

		double scale = 1.0;
		for (int i = 0; i < size; i++) {
			double g = 65535.0 * i / size * cutoff;
			if (i > size * cutoff)
				g += 65535.0 * (1 - cutoff);
			g *= scale;
			crtc_gamma->red[i] = g;
			crtc_gamma->green[i] = g;
			crtc_gamma->blue[i] = g;
		}
		XRRSetCrtcGamma(dpy, crtcxid, crtc_gamma);

		XFree(crtc_gamma);
	}
}

__dead void
usage(void)
{
        extern char     *__progname;

        fprintf(stderr, "usage: %s [-s <percentage>]\n", __progname);
        exit(1);
}

int
main(int argc, char **argv)
{
        if (pledge("dns exec proc rpath stdio unix", NULL) == -1) {
                err(1, "pledge");
        }

	Display *dpy = XOpenDisplay(NULL);
	int screen = DefaultScreen(dpy);
	Window root = RootWindow(dpy, screen);
	XEvent ev;
	int dim = 0;
	int olddim = -1;
	double brightness;
        int lowbrightness = 20;
        int highbrightness;
        int c;


        while ((c = getopt(argc, argv, "s:")) != -1) {
                switch(c) {
                        case 's':
                                lowbrightness = atoi(optarg);
                                break;
                        default:
                                usage();
                }
        }

        if (lowbrightness <= 0) {
                fprintf(stderr, "brightness must be strictly positive\n");
                exit(2);
        }
        highbrightness = 2 * lowbrightness;

	XSelectInput(dpy, root, FocusChangeMask);

	while (1) {
		XNextEvent(dpy, &ev);

		brightness = getbrightness(dpy, screen, root);

		if (brightness > 400)
			dim = 1;
		else
			dim = 0;

		if (dim != olddim) {
			setbrightness(dim ? lowbrightness : highbrightness);

			/* adjustgamma(dpy, root, dim ? 0.5 : 1.0); */
		}
		olddim = dim;
	}
}
