#include "utils.h"
#include <stdio.h>

int alloc_color_from_hex(Display *display, Colormap colormap, const char *hex_color, XColor *xcolor) {
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
