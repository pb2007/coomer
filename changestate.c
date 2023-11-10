// --- util functions ---
Bool getCursorPos(int* x, int* y) {
	// create a trash memory address to throw things I don't care about
	if (!def.garbage) 
		def.garbage = malloc(MAX(sizeof(Window), sizeof(int)));
	return XQueryPointer(def.dis, def.win,
		def.garbage, def.garbage, def.garbage, def.garbage,
		x, y,
		def.garbage
	);
}

void generatePolygon(Points* p,
	double xc, double yc, double x, double y, int sides
) {
	p->count = 0;
	p->capacity = 0;

	double x1 = 0, y1 = 0;

	double r = sqrt((x-x1)*(x-x1) + (y-y1)*(y-y1));
	double a = atan2(y-y1, x-x1);

	for (int i = 0; i <= sides; i++) {
		x1 = x; y1 = y;
		a = a + M_PI * 2 / sides;
		x = xc + r * cos(a);
		y = yc + r * sin(a);
		if (i > 0) {
			nob_da_append(p, ((XPointDouble){ x1, y1 }));
			nob_da_append(p, ((XPointDouble){ x,  y  }));
		}
	}
}

// --- change state ---
void calculateScale(void) {
	double scaleX = 1.0 / ((double) s.winW / def.img->width)  / s.scale;
	double scaleY = 1.0 / ((double) s.winH / def.img->height) / s.scale;

#if SCALE_MODE
	// fit to window
	double winScale = 
		(s.winW * def.img->height > s.winH * def.img->width)
		? scaleY : scaleX;
	scaleX = winScale; scaleY = winScale;
#endif

	s.xdt.matrix[0][0] = scaleX;
	s.xdt.matrix[1][1] = scaleY;
	s.xt = XdtToXT(s.xdt);
}

void calculateDragging(void) {
	double dX, dY;
	if ((s.modkeys & ShiftMask) && !(s.modkeys & WM_MOD)) {
		dX = s.mousePrevX - s.mouseCurX;
		dY = s.mousePrevY - s.mouseCurY;
	} else {
		dX = (s.mousePrevX/s.scale) - (s.mouseCurX/s.scale);
		dY = (s.mousePrevY/s.scale) - (s.mouseCurY/s.scale);
	}

	s.dragX += dX; s.dragY += dY;

	s.xdt.matrix[0][2] = s.dragX + s.zoomX;
	s.xdt.matrix[1][2] = s.dragY + s.zoomY;
	s.xt = XdtToXT(s.xdt);
}

void setScaleTxt(double scale) {
	if (scale > 10) {
		scale = round(scale);
		snprintf(s.scaletxt, SCALETXT_LEN, "%.0fx", scale);
	} else {
		snprintf(s.scaletxt, SCALETXT_LEN, "%.2fx", scale);
	}

	XGlyphInfo info;
	XftTextExtents8(
		def.dis, def.font, 
		(FcChar8*)s.scaletxt, MIN(strlen(s.scaletxt), SCALETXT_LEN),
		&info
	);
	s.scaletxtW = info.width;
	s.drawDirty = true;
}


#define ZOOM_IN  true
#define ZOOM_OUT false

void zoomFl(bool in) {
	if (in) s.radius *= SCALE_FACTOR;
	else    s.radius /= SCALE_FACTOR;
	s.radius = fmin(fmax(s.radius, 1.0), 300.0);	

	generatePolygon(&def.p, 
		s.radius*2, s.radius*2, s.radius, s.radius, 
		FL_RESOLUTION
	);
	XRenderComposite(
		def.dis, PictOpClear,
		def.maskPic, None, def.maskPic,
		0, 0, 0, 0, 0, 0,
		def.img->width, def.img->height
	);

	snprintf(s.cmdbar, CMDBAR_LEN, "%s flashlight",
		(in) ? "Dilate" : "Constrict"
	);

	s.drawDirty = true;
}

// TODO scaling {
// 	use winscale rather than camscale to scale correctly when
// 	the window aspect ratio does not match the image
// }
void zoom(bool in) {
	if (s.modkeys & ControlMask)
		return zoomFl(in);

	getCursorPos(&s.mouseCurX, &s.mouseCurY);

	if (in) s.scale *= SCALE_FACTOR;
	else    s.scale /= SCALE_FACTOR;
	s.scale = fmin(fmax(s.scale, 0.12), 256.0);
	setScaleTxt(s.scale);

	calculateScale();

	s.zoomX = (1-(1/s.scale)) * s.mouseCurX;
	s.zoomY = (1-(1/s.scale)) * s.mouseCurY;

	s.xdt.matrix[0][2] = s.zoomX + s.dragX;
	s.xdt.matrix[1][2] = s.zoomY + s.dragY;
	s.xt = XdtToXT(s.xdt);

	snprintf(s.cmdbar, CMDBAR_LEN, "Zoom %s", (in) ? "in" : "out");

	s.drawDirty = true;
	s.mousePrevX = s.mouseCurX; s.mouseCurY = s.mousePrevY;
}

void resetState(void) {
	s.dragging   = false;
	s.flashlight = false;
	s.drawCmdBar = true;
	s.scale      = 1.0;
	s.radius     = 0.0;
	s.dragX      = 0;
	s.dragY      = 0;
	s.zoomX      = 0;
	s.zoomY      = 0;
	s.xdt        = XdtIdentity();
	s.xt         = XdtToXT(s.xdt);

	setScaleTxt(s.scale);

	getCursorPos(&s.mouseCurX, &s.mouseCurY);
	s.mousePrevX = s.mouseCurX;
	s.mousePrevY = s.mousePrevY;

	def.p.count = 0;
	def.p.capacity = 0;

	strncpy(s.cmdbar, "Reset", CMDBAR_LEN);

	// recalculate, because 1.0 cam scale is not necessarily
	// the window scale we need.
	calculateScale();
	redraw();

	s.lastRadius = s.radius;
}
