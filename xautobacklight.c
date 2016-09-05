#include <X11/extensions/Xrandr.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>

pid_t
setbrightness(int percent)
{
        char *argv[] = { "xbacklight", "-set", "100", NULL };
        char percent_s[3];
        pid_t pid;

        sprintf(percent_s, "%d", percent);
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

int
main(int argc, char **argv)
{
	Display *dpy = XOpenDisplay(NULL);
	int screen = DefaultScreen(dpy);
	Window root = RootWindow(dpy, screen);
	XEvent ev;
	int dim = 0;
	int olddim = -1;
	double brightness;

	XSelectInput(dpy, root, FocusChangeMask);

	while (1) {
		XNextEvent(dpy, &ev);

		brightness = getbrightness(dpy, screen, root);

		if (brightness > 400)
			dim = 1;
		else
			dim = 0;

		if (dim != olddim) {
			setbrightness(dim ? 20 : 40);

			/* adjustgamma(dpy, root, dim ? 0.5 : 1.0); */
		}
		olddim = dim;
	}
}
