#ifndef UTILS_H
#define UTILS_H

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

int alloc_xcolor_from_hex(Display *display, Colormap colormap, const char *hex_color, XColor *xcolor);
int alloc_xftcolor_from_hex(Display *display, Visual *visual, Colormap colormap, const char *hex_color, XftColor *xft_color);
#endif