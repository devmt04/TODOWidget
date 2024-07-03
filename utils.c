#include "utils.h"
#include <stdio.h>

int alloc_xcolor_from_hex(Display *display, Colormap colormap, const char *hex_color, XColor *xcolor) {
    unsigned int r, g, b;
    int success;

    // Convert HEX color to RGB values
    if (sscanf(hex_color, "#%02x%02x%02x", &r, &g, &b) != 3) {
        fprintf(stderr, "Invalid HEX color format: %s\n", hex_color);
        return 0;
    }

    // Scale RGB values from 0-255 to 0-65535 (X11 range)
    xcolor->red = r * 257;   // 65535 / 255 = 257
    xcolor->green = g * 257; // 65535 / 255 = 257
    xcolor->blue = b * 257;  // 65535 / 255 = 257
    xcolor->flags = DoRed | DoGreen | DoBlue;

    // Allocate the color from the colormap
    success = XAllocColor(display, colormap, xcolor);
    if (!success) {
        fprintf(stderr, "Failed to allocate color from HEX: %s\n", hex_color);
        return 0;
    }

    return 1;
}

int alloc_xftcolor_from_hex(Display *display, Visual *visual, Colormap colormap, const char *hex_color, XftColor *xft_color){
    unsigned int r, g, b, a;
    int success;

    // Convert HEX color to RGB values
    if (sscanf(hex_color, "#%02x%02x%02x%02x", &r, &g, &b, &a) != 4) {
        fprintf(stderr, "Invalid HEX color format: %s\n", hex_color);
        return 0;
    }
    XRenderColor xrcolor;
    // Scale RGB values from 0-255 to 0-65535 (X11 range)
    xrcolor.red = r * 257;   // 65535 / 255 = 257
    xrcolor.green = g * 257; // 65535 / 255 = 257
    xrcolor.blue = b * 257;  // 65535 / 255 = 257
    xrcolor.alpha = a * 257;
    // Allocate the color from the colormap
    success = XftColorAllocValue(display, visual, colormap, &xrcolor, xft_color);
    if (!success) {
        fprintf(stderr, "Failed to allocate color from HEX: %s\n", hex_color);
        return 0;
    }
    return 1;
}