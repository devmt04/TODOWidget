#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>

#include "main.h"
#include "utils.h"
#include "pixmaps/window_mask.xbm"


#include <stdio.h>
#include <stdlib.h>

void open_display();
void alloc_colors();
void createAndMap_root_window(Window *root_win, int argc, char **argv);
void start_event_loop();

Display *display;
int screen_num;
int display_width, display_height;
char *progname;
XColor bgcolor; // #BAE500

int main(int argc, char **argv){
	progname = argv[0];

	Window root_win;
	
	open_display();
	alloc_colors();
	createAndMap_root_window(&root_win, argc, argv);
	start_event_loop();

	XCloseDisplay(display);
	return 0;
}

void open_display(){
	// Open Display
	if((display = XOpenDisplay(NULL)) == NULL){
		fprintf(stderr, "Error Opening Display\nExiting..\n");
		exit(-1);
	}else{
		screen_num = DefaultScreen(display);
		// Get full display width and height
		display_width = DisplayWidth(display, screen_num);
		display_height = DisplayHeight(display, screen_num);
		printf("Successfully connect to display server\nDisplay Width: %d Display Height: %d\n", display_width, display_width);
	}
}

void alloc_colors(){
	// Get Colormap and allocate colors
	Colormap colormap = DefaultColormap(display, screen_num);
	// Allocate bgcolor
	if(!alloc_color_from_hex(display, colormap, "#BAE500", &bgcolor)){
		fprintf(stderr, "Failed allocating BGColor\nexiting...\n");
		exit(-1);
	}
}

void createAndMap_root_window(Window *root_win, int argc, char **argv){
	
	// Creating Root Window
	int root_x, root_y, root_width, root_height;
	root_width = 250;
	root_height = 150;
	root_x = display_width - root_width - 5;
	root_y = 8;
	*root_win = XCreateSimpleWindow(display, RootWindow(display, screen_num), root_x, root_y, root_width, root_height, 2, BlackPixel(display, screen_num), WhitePixel(display, screen_num));

	// Motif Hints for root window : appliying NO DECORATIONS
	Atom mwmHintsProperty = XInternAtom(display, "_MOTIF_WM_HINTS", False);
	MotifWmHints hints;
	hints.flags = MWM_HINTS_DECORATIONS;
	hints.decorations = 0 ;
	XChangeProperty(display, *root_win, mwmHintsProperty, mwmHintsProperty, 32,
                   PropModeReplace, (unsigned char *)&hints, sizeof(hints)/4);


	// Allocating Size Hints
	XSizeHints *root_size_hints;
	if(!(root_size_hints = XAllocSizeHints())){
		fprintf(stderr, "failure allocating memory\n");
		exit(-1);
	}
	root_size_hints->flags = PPosition | PSize | PMinSize | PMaxSize;
	root_size_hints->min_width = root_width;
	root_size_hints->min_height = root_height;
	root_size_hints->max_width = root_width;
	root_size_hints->max_height = root_height;

	// Allocating WM hints
	XWMHints *wm_hints;
	if(!(wm_hints = XAllocWMHints())){
		fprintf(stderr, "failure allocating memory\n");
		exit(-1);
	}
	wm_hints->flags = StateHint | InputHint;
	wm_hints->initial_state = NormalState;
	wm_hints->input = True;

	// Allocating class hints
	XClassHint *class_hints;
	if(!(class_hints = XAllocClassHint())){
		fprintf(stderr, "failure allocating memory\n");
		exit(-1);
	}
	class_hints->res_name = progname;
	class_hints->res_class = "TODO Widget";

	XTextProperty windowName, iconName;
	char *window_name = "TODO Widget";
	char *icon_name = "TODO Widget";

	if(XStringListToTextProperty(&window_name,1,&windowName)==0){
		fprintf(stderr, "%s: strcture allocation for windowName failed\n", progname);
		exit(-1);
	}
	if(XStringListToTextProperty(&icon_name,1,&iconName)==0){
		fprintf(stderr, "%s: strcture allocation for iconName failed\n", progname);
		exit(-1);
	}

	XSetWMProperties(display, *root_win, &windowName, &iconName, argv, argc, root_size_hints, wm_hints, class_hints);

	// Selecting Types of Input for Root Window
	XSelectInput(display, *root_win, ExposureMask);

	Window child;
	child = XCreateSimpleWindow(display, *root_win, 0, 0, 250, 50, 2, BlackPixel(display, screen_num), bgcolor.pixel);;
	Pixmap shape_mask;
	GC gc;
	XGCValues values;

	shape_mask =  XCreateBitmapFromData(display, child, 
		window_mask_bits, window_mask_width, window_mask_height); // depth - 1, i.e, Bitmap
	
	XShapeCombineMask(display, child, ShapeBounding, 0, 0, shape_mask, ShapeSet);
	XFreePixmap(display, shape_mask);

	XSelectInput(display, child, ExposureMask | ButtonPressMask | KeyPressMask);
	XMapWindow(display, *root_win);
	XMapWindow(display, child);
}

void start_event_loop(){
	XEvent report;
 	//XSetInputFocus(display, RootWindow(display, DefaultScreen(display)), RevertToParent, CurrentTime);
	while(1){
		XNextEvent(display, &report);
		switch(report.type){
		case Expose:
			printf("Exposed\n");
			break;
		case ButtonPress:
			printf("Button Pressed\n");
			break;
		case KeyPress:
			printf("key\n");
			break;
		default:
			break;
		}
		fflush(stdout); 
	}
}