#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <sys/param.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <X11/extensions/Xrender.h>
#include <X11/Xft/Xft.h>

#include "mininob.h"
#include "xdoubletransform.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "config.h"

typedef struct {
	XPointDouble* items;
	size_t count;
	size_t capacity;
} Points;

typedef struct {
	char** items;
	size_t count;
	size_t capacity;
} Tokens;

static struct Deferables {
	Display* dis;
	Window win;
	XClassHint* hint;

	XImage* img;
	Pixmap  pix, maskPix;

	Picture imgPic, winPic, maskPic;
	Picture colWhitePic;

	XftFont*  font; 
	XftDraw*  xft;
	XftColor* cxftText;

	XIM xim;
	XIC xic;

	Points p;
	Tokens t;

	void* garbage; // size max(Window (prob card32), int)
} def = {0};

static struct State {
	XDoubleTransform xdt;
	XTransform xt;

	double scale;
	double zoomX, zoomY;
	double dragX, dragY;

	int winW, winH;
	int mousePrevX, mousePrevY, mouseCurX, mouseCurY;
	int barH;

	unsigned int modkeys;
	bool dragging;

	bool flashlight;
	double radius, lastRadius;

	bool drawDirty;
	bool drawCmdBar;

	bool quit;

	bool cmdMode;
#define CMDBAR_LEN 255
	char   cmdbar[CMDBAR_LEN+1];
	size_t cmdcur;

#define SCALETXT_LEN 6
	char scaletxt[SCALETXT_LEN+1];
	unsigned short scaletxtW;
} s = {0};

static const XRenderColor colWhite = {0xffff, 0xffff, 0xffff, 0xffff};

void cleanup(void) {
	Display* d = def.dis;
	if (d) {
		int      scr  = DefaultScreen(d);
		Visual*  vi   = DefaultVisual(d, scr);
		Colormap cmap = DefaultColormap(d, scr);

		if (def.imgPic)      XRenderFreePicture(d, def.imgPic);
		if (def.winPic)      XRenderFreePicture(d, def.winPic);
		if (def.maskPic)     XRenderFreePicture(d, def.maskPic);
		if (def.colWhitePic) XRenderFreePicture(d, def.colWhitePic);

		if (def.pix)         XFreePixmap(d, def.pix);
		if (def.maskPix)     XFreePixmap(d, def.maskPix);

		if (def.font)        XftFontClose(d, def.font);
		if (def.cxftText)    XftColorFree(d, vi, cmap, def.cxftText);
		if (def.xft)         XftDrawDestroy(def.xft);

		if (def.xic)	     XDestroyIC(def.xic);
		if (def.xim)	     XCloseIM(def.xim);

		if (def.win)  {
			XUnmapWindow(d, def.win);
		    	XDestroyWindow(d, def.win);
		}
		
	        XCloseDisplay(d);
	}

	// nothing that uses the display may be after here

	if (def.hint)          XFree(def.hint);
	if (def.img)           XDestroyImage(def.img);

	if (def.p.items)       free(def.p.items);
	if (def.t.items)       free(def.t.items);
	if (def.garbage)       free(def.garbage);
}

void redraw(void) {
	XRenderSetPictureTransform(def.dis, def.imgPic, &s.xt);

	XRenderComposite(
		def.dis, PictOpOver,
		def.imgPic, None, def.winPic, // src, mask, dest
		0, 0, // src x y offset
		0, 0, // mask
		0, 0, // dest
		def.img->width, def.img->height
	);

	if (s.flashlight) {
		XRenderFillRectangle(
			def.dis, PictOpOver,
			def.winPic, &colFl,
			0, 0, def.img->width, def.img->height
		);

		if (def.p.count) {
			XRenderCompositeDoublePoly(
				def.dis, PictOpOver,
				def.colWhitePic, def.maskPic,
				NULL,
				0, 0, 0, 0,
				def.p.items, def.p.count, 0
			);

			XRenderComposite(
				def.dis, PictOpOver,
				def.imgPic, def.maskPic, def.winPic,
				s.mouseCurX-s.radius*2,
				s.mouseCurY-s.radius*2,
				0, 
				0,
				s.mouseCurX-s.radius*2,
				s.mouseCurY-s.radius*2,
				def.img->width, def.img->height
			);
		}

		if (s.radius != s.lastRadius)
			s.lastRadius = s.radius;
	}

	if (s.drawCmdBar) {
		XRenderFillRectangle(def.dis, PictOpOver, 
			def.winPic, &colBar,
			0, s.winH-s.barH,
			s.winW, s.winH
		);

		XftDrawString8(
			def.xft, def.cxftText, def.font, 
			0, 
			s.winH - (s.barH/4),
			(FcChar8*)s.cmdbar, 
			MIN(strlen(s.cmdbar), CMDBAR_LEN)
		);
		XftDrawString8(
			def.xft, def.cxftText, def.font,
			s.winW - s.scaletxtW,
			s.winH - (s.barH / 4),
			(FcChar8*)s.scaletxt, 
			MIN(strlen(s.scaletxt), SCALETXT_LEN)
		);

		// TODO {
		//	get cursor position in pixels
		//	draw cursor -- line, or maybe '_' -- at that position
		// }
	}

	s.drawDirty = false;
}

#include "changestate.c"
#include "commands.c"
#include "events.c"

#define DIE(exitcode, ...) \
	{\
	fprintf(stderr, __VA_ARGS__);\
	exit(exitcode);\
	}

int main(void) {
	atexit(cleanup);

	Display* d = XOpenDisplay(NULL);
	def.dis = d;
	if (!d) DIE(1, "error: could not get display");

	int eventBase, errorBase, majVer, minVer;
	if (!XRenderQueryExtension(d, &eventBase, &errorBase))
		DIE(1, "error: you don't have XRender");
	XRenderQueryVersion(d, &majVer, &minVer);
	// TODO: find out what minimum version we need & verify it

	int      scr  = DefaultScreen(d);
	Visual*  vi   = DefaultVisual(d, scr);
	Colormap cmap = DefaultColormap(d, scr);

	Window root = DefaultRootWindow(d);

	// take screenshot
	XWindowAttributes attr = {0};
	XGetWindowAttributes(d, root, &attr);

	XImage* img = XGetImage(
		d, root,
		0, 0,
		attr.width, attr.height,
		AllPlanes, ZPixmap
	);
	def.img = img;
	if (!img) DIE(1, "error: could not screenshot");

	// --- create window ---
	Window win = XCreateSimpleWindow(d, root,
		0, 0,
		attr.width, attr.height,
		0,
		0, 0
	);
	def.win = win;

	s.winW = attr.width;
	s.winH = attr.height;

	// set window and class name
	def.hint = XAllocClassHint();
	if (!def.hint) DIE(1, "error: could not make hint -- buy more ram");
	def.hint->res_name =  "Coomer";
	def.hint->res_class = "coomer";
	XSetClassHint(d, win, def.hint);
	XStoreName(d, win, def.hint->res_name);

	// subscribe to quit atom and events
	Atom quitAtom = XInternAtom(d, "WM_DELETE_WINDOW", false);
	XSetWMProtocols(d, win, &quitAtom, 1);
	XSelectInput(d, win, 
		ExposureMask | 
		KeyPressMask | KeyReleaseMask |
		ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
		StructureNotifyMask
	);

	XMapWindow(d, win);

	// --- create drawing resources --- 
	
	// xft
	def.font = XftFontOpenName(d, scr, fontname);
	if (!def.font) 
		DIE(1, "error: couldn't make the font");
	s.barH = def.font->ascent + def.font->descent;

	XftColor cxftText = {0}; def.cxftText = &cxftText;
	if (!(XftColorAllocValue(d, vi, cmap, &colText, def.cxftText)))
		DIE(1, "error: couldn't make text color");

	def.xft = XftDrawCreate(d, win, vi, cmap);
	
	// xrender 
	XRenderPictFormat* format = 
		XRenderFindVisualFormat(d, vi);
	XRenderPictFormat* alpha  =
		XRenderFindStandardFormat(d, PictStandardA8);
	if (!format || !alpha)
		DIE(1, "error: could not find a picture format");

	def.pix = XCreatePixmap(d, win,
		img->width, img->height, img->depth
	);
	def.maskPix = XCreatePixmap(d, win,
		img->width, img->height, alpha->depth
	);

	def.imgPic    = XRenderCreatePicture(d, def.pix, format, 0, NULL);
	def.maskPic   = XRenderCreatePicture(d, def.maskPix, alpha, 0, NULL);
	def.winPic    = XRenderCreatePicture(d, win, format, 0, NULL);

	// initialize resources
	XRenderComposite(
		def.dis, PictOpClear,
		def.maskPic, None, def.maskPic,
		0, 0, 0, 0, 0, 0,
		def.img->width, def.img->height
	);

	GC tmpgc = XCreateGC(d, def.pix, 0, NULL);
	XPutImage(d, def.pix, tmpgc, img, 0, 0, 0, 0, img->width, img->height);
	XFreeGC(d, tmpgc);

	Pixmap tmpPix = XCreatePixmap(d, win, 1, 1, img->depth);
	Picture colWhitePic = XRenderCreatePicture(
		d, tmpPix, format, CPRepeat,
		&(XRenderPictureAttributes){ .repeat = 1 }
	);
	def.colWhitePic = colWhitePic;
	XRenderFillRectangle(
		d, PictOpOver,
		colWhitePic, &colWhite, 0, 0, 1, 1
	);
	XFreePixmap(d, tmpPix);

	// --- create input resources ---
	if ((def.xim = XOpenIM(d, NULL, NULL, NULL)) == NULL)
		DIE(1, "error: could not open input device");
	
	def.xic = XCreateIC(def.xim,
		XNInputStyle,   XIMPreeditNothing | XIMStatusNothing,
		XNClientWindow, win,
		XNFocusWindow,  win,
		NULL
	);

	// --- initialize state ---
	getCursorPos(&s.mouseCurX, &s.mouseCurY);
	s.mousePrevX = s.mouseCurX; s.mousePrevY = s.mouseCurY;

	resetState();
	memset(s.cmdbar, 0, CMDBAR_LEN);
	
	// --- event loop ---
	XEvent ev;
	while (!s.quit) {
		do {
			XNextEvent(d, &ev);
			handleEvent(ev, quitAtom);
		 } while (XPending(d) > 0);

		if (s.drawDirty) {
			clock_t start = clock();

			redraw();

			clock_t end = clock();
			double frametime = (double)(end-start)/CLOCKS_PER_SEC;
			useconds_t ftus = (useconds_t)(frametime*1000000);
			if (ftus < limitTime) usleep(limitTime - ftus);
		}
	}

	return 0;
}
