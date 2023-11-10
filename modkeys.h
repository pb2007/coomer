#ifndef _MODKEYS_H
#define _MODKEYS_H

#include <X11/Xlib.h>

unsigned int MkOnKeyPress(KeySym k, unsigned lastModkeys) {
	unsigned res = lastModkeys;
	switch (k) {
		case XK_Shift_L:
		case XK_Shift_R:
			res |= ShiftMask; break;
		case XK_Control_L:
		case XK_Control_R:
			res |= ControlMask; break;
		case XK_Alt_L:
		case XK_Alt_R:
			res |= Mod1Mask; break;
		case XK_Super_L:
		case XK_Super_R:
			res |= Mod4Mask; break;
	}
	return res;
}

unsigned int MkOnKeyRelease(KeySym k, unsigned lastModkeys) {
	unsigned res = lastModkeys;
	switch (k) {
		case XK_Shift_L:
		case XK_Shift_R:
			res &= ~ShiftMask; break;
		case XK_Control_L:
		case XK_Control_R:
			res &= ~ControlMask; break;
		case XK_Alt_L:
		case XK_Alt_R:
			res &= ~Mod1Mask; break;
		case XK_Super_L:
		case XK_Super_R:
			res &= ~Mod4Mask; break;
	}
	return res;
}


#endif
