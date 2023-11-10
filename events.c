#include "modkeys.h"

void insert(const char *str, ssize_t n) {
	if (strlen(s.cmdbar) + n > sizeof(s.cmdbar) - 1)
		return;

	/* move existing text out of the way, insert new text, and update cursor */
	memmove(
		&s.cmdbar[s.cmdcur + n], 
		&s.cmdbar[s.cmdcur], 
		sizeof(s.cmdbar) - s.cmdcur - MAX(n, 0)
	);
	if (n > 0)
		memcpy(&s.cmdbar[s.cmdcur], str, n);

	s.cmdcur += n;
}

void onKeyPress(KeySym k) {
	switch (k) {
	case XK_q: s.quit = true; break;

	case XK_plus:
	case XK_equal:
	case XK_KP_Add:
	case XK_i:
		zoom(ZOOM_IN); break;
	case XK_minus:
	case XK_KP_Subtract:
	case XK_o:
		zoom(ZOOM_OUT); break;
	case XK_0:
	case XK_KP_0:
	case XK_r:
		resetState(); break;
	case XK_f:
		s.flashlight = !s.flashlight;
		snprintf(s.cmdbar, CMDBAR_LEN,
			"Flashlight %s",
			(s.flashlight) ? "on" : "off"
		);
		s.drawDirty = true;
		break;
	case XK_semicolon:
	case XK_colon:
		memset(s.cmdbar, 0, CMDBAR_LEN);
		memset(s.scaletxt, 0, SCALETXT_LEN);
		s.cmdbar[0] = ':';
		s.cmdcur = 1;

		s.cmdMode = true;
		s.drawDirty = true;
		break;
	}
}

void onKeyPressCmd(XKeyEvent* ev) {
	char buf[32];
	int len;
	KeySym k;
	Status status;

	len = XmbLookupString(def.xic, ev, buf, sizeof(buf), &k, &status);

	switch (status) {
	default: /* XLookupNone, XBufferOverflow */ 
		return;
	case XLookupChars: 
		goto insert;
	case XLookupKeySym:
	case XLookupBoth:
		break;
	}

	switch (k) {
	default:
insert:
		if (!iscntrl(*buf))
			insert(buf, len);
		break;

	case XK_Left:
		if (s.cmdcur > 1) s.cmdcur--;
		break;
	case XK_Right:
		if (s.cmdbar[s.cmdcur] != '\0') s.cmdcur++;
		break;

	case XK_Delete:
		if (s.cmdbar[s.cmdcur] == '\0') break;
		s.cmdcur++;
		// fallthrough
	case XK_BackSpace:
		if (s.cmdcur <= 1)  {
			s.cmdcur = 1;
			break;
		}
		insert(NULL, -1);
		break;

	case XK_Return:
	case XK_KP_Enter:
		tokenize(&def.t, s.cmdbar+1, " ");
		parse(&def.t);

		setScaleTxt(s.scale);
		s.cmdMode = false;
		break;

	case XK_Escape:
		memset(s.cmdbar, 0, CMDBAR_LEN);
		setScaleTxt(s.scale);
		s.cmdMode = false;
		break;
	}

	s.drawDirty = true;
}

void handleEvent(XEvent ev, Atom quitAtom) {
	switch (ev.type) {
	case Expose: {
		redraw();
		break;
	}
	case ConfigureNotify: {
		s.winW = ev.xconfigure.width;
		s.winH = ev.xconfigure.height;

		calculateScale();
		redraw();
		break;
	}
	case KeyPress: {
		KeySym k = XLookupKeysym(&ev.xkey, 0);

		if (s.cmdMode) onKeyPressCmd(&ev.xkey);
		else           onKeyPress(k);

		s.modkeys = MkOnKeyPress(k, s.modkeys);
		break;
	}
	case KeyRelease: {
		KeySym k = XLookupKeysym(&ev.xkey, 0);
		s.modkeys = MkOnKeyRelease(k, s.modkeys);
		break;
	}
	case MotionNotify: {
		s.mouseCurX = ev.xmotion.x;
		s.mouseCurY = ev.xmotion.y;

		if (s.dragging) 
			calculateDragging();
		if (s.dragging || s.flashlight )
			s.drawDirty = true;

		s.mousePrevX = s.mouseCurX;
		s.mousePrevY = s.mouseCurY;
		break;
	}
	case ButtonPress: {
		getCursorPos(&s.mouseCurX, &s.mouseCurY);
		s.mousePrevX = s.mouseCurX; 
		s.mousePrevY = s.mouseCurY;

		switch (ev.xbutton.button) {
		case Button1: s.dragging = true; break;
		case Button4: zoom(ZOOM_IN);     break;
		case Button5: zoom(ZOOM_OUT);    break;
		}
		break;
	};
	case ButtonRelease: {
		switch (ev.xbutton.button) {
		case Button1: s.dragging = false; break;
		}
		break;
	}
	case ClientMessage: {
		if ((Atom)ev.xclient.data.l[0] == quitAtom)
			s.quit = true;
		break;
	}
	} // switch
}
