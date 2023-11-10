// the scale will multiply by this factor each time
// eg. if it is 2, scale will go 50% -> 100% -> 200%, "2x"ing each time
#define SCALE_FACTOR 1.5

// 0 = stretch
// 1 = fit
#define SCALE_MODE 1

// negate keys getting "stuck" when doing some window manager operations
#define WM_MOD Mod4Mask

// flashlight "circle" resolution (no. of sides)
// higher number means the shape will be more circular but could use more
// resources
#define FL_RESOLUTION 50

// maximum amount of redraws allowed per second
// a lower number means less resources will be used but updating will be
// choppier
#define FPS_LIMIT 60
static const useconds_t limitTime = 1000000 / FPS_LIMIT;

static const char* fontname = "monospace:size=12";

// colors. max is 65535, not 255.    red     green   blue    alpha
static const XRenderColor colBar  = {0,      0,      0,      .60*65535};
static const XRenderColor colFl   = {0,      0,      0,      .80*65535};
static const XRenderColor colText = {65535,  65535,  65535,  65535};

