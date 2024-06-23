#ifndef UTILS_H
#define UTILS_H

#include <X11/Xlib.h>
int alloc_color_from_hex(Display *display, Colormap colormap, const char *hex_color, XColor *xcolor);
#endif