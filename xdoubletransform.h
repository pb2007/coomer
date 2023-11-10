#ifndef _XDOUBLETRANSFORM
#define _XDOUBLETRANSFORM

#include <X11/Xlib.h>

typedef struct {
	double matrix[3][3];
} XDoubleTransform;

#define XdtIdentity() (XDoubleTransform){{{1.0,0,0},{0,1.0,0},{0,0,1.0}}}

XTransform XdtToXT(XDoubleTransform xdt) {
	XTransform xt = {{
		{
			XDoubleToFixed(xdt.matrix[0][0]),
			XDoubleToFixed(xdt.matrix[0][1]),
			XDoubleToFixed(xdt.matrix[0][2])
		},
		{
			XDoubleToFixed(xdt.matrix[1][0]),
			XDoubleToFixed(xdt.matrix[1][1]),
			XDoubleToFixed(xdt.matrix[1][2])
		},
		{
			XDoubleToFixed(xdt.matrix[2][0]),
			XDoubleToFixed(xdt.matrix[2][1]),
			XDoubleToFixed(xdt.matrix[2][2])
		},
	}};
	return xt;
}

XDoubleTransform XTtoXdt(XTransform xt) {
	XDoubleTransform xdt = {{
		{
			XFixedToDouble(xt.matrix[0][0]),
			XFixedToDouble(xt.matrix[0][1]),
			XFixedToDouble(xt.matrix[0][2])
		},
		{
			XFixedToDouble(xt.matrix[1][0]),
			XFixedToDouble(xt.matrix[1][1]),
			XFixedToDouble(xt.matrix[1][2])
		},
		{
			XFixedToDouble(xt.matrix[2][0]),
			XFixedToDouble(xt.matrix[2][1]),
			XFixedToDouble(xt.matrix[2][2])
		},
	}};
	return xdt;
}
#endif
