#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>

#include "main.h"
#include "utils.h"

#include "pixmaps/tile_window_mask_250x75.xbm"
#include "pixmaps/tile_window_mask_250x55.xbm"
#include "pixmaps/tile_window_mask_250x35.xbm"
#include "pixmaps/addnewbtn_window_mask.xbm"
#include "pixmaps/addnewbtn_icon.xbm"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NOTE_COUNT 0

void open_display();
void alloc_colors();
void createAndMap_root_window(int argc, char **argv);
void start_event_loop();
void createInitialMsgWindow();
void drawInitialMsgString();
void draw_string_on_window(Window *win, GC *gc, int x, int y, char *string, int strlength);
void drawAddNewBtn();

Display *display;
int screen_num;
int display_width, display_height;
char *progname;
XColor bgcolor; // #747474
XColor tilecolor; // #C8B4B4
XColor addnewbtncolor;

XFontStruct *font_info;

Window root_win, initialmsg_window, addbtn_win;

int isInitialWindowMapped = 0;

int main(int argc, char **argv){
	progname = argv[0];

	open_display();

	// Loading fonts
	char *fontname = "9x15";
	if((font_info = XLoadQueryFont(display, fontname)) == NULL ){
		printf("Error loading 9x15 fonts\nexiting...\n");
		exit(-1);
	}

	alloc_colors();
	createAndMap_root_window(argc, argv);
	createInitialMsgWindow();
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
	if(!alloc_color_from_hex(display, colormap, "#747474", &bgcolor)){
		fprintf(stderr, "Failed allocating BGColor\nexiting...\n");
		exit(-1);
	}
	if(!alloc_color_from_hex(display, colormap, "#C8B4B4", &tilecolor)){
		fprintf(stderr, "Failed allocating BGColor\nexiting...\n");
		exit(-1);
	}
	if(!alloc_color_from_hex(display, colormap, "#C8B4B4", &addnewbtncolor)){
		fprintf(stderr, "Failed allocating BGColor\nexiting...\n");
		exit(-1);
	}
}

void createAndMap_root_window(int argc, char **argv){
	
	// Creating Root Window
	int root_x, root_y, root_width, root_height;
	root_width = 250;
	root_height = 150;
	root_x = display_width - root_width - 5;
	root_y = 8;
	root_win = XCreateSimpleWindow(display, RootWindow(display, screen_num), root_x, root_y, root_width, root_height, 2, BlackPixel(display, screen_num), bgcolor.pixel);

	// Motif Hints for root window : appliying NO DECORATIONS
	Atom mwmHintsProperty = XInternAtom(display, "_MOTIF_WM_HINTS", False);
	MotifWmHints hints;
	hints.flags = MWM_HINTS_DECORATIONS;
	hints.decorations = 0 ;
	XChangeProperty(display, root_win, mwmHintsProperty, mwmHintsProperty, 32,
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

	XSetWMProperties(display, root_win, &windowName, &iconName, argv, argc, root_size_hints, wm_hints, class_hints);


	// Creating a button window to add items in TODO list
	int btn_x, btn_y, btn_width, btn_height;
	btn_width = 31;
	btn_height = 31;
	btn_x = root_width - btn_width - 15;
	btn_y = root_height - btn_height - 15;
	
	addbtn_win = XCreateSimpleWindow(display, root_win, btn_x,btn_y,btn_width,btn_height,1,BlackPixel(display, screen_num), addnewbtncolor.pixel);
	
	Pixmap addbtn_shapemask = XCreateBitmapFromData(display, addbtn_win, addnewbtn_window_mask_bits, addnewbtn_window_mask_width, addnewbtn_window_mask_height);

	XShapeCombineMask(display, addbtn_win, ShapeBounding, 0,0,addbtn_shapemask, ShapeSet);
	XFreePixmap(display, addbtn_shapemask);

	// Selecting Types of Input for Root Window
	XSelectInput(display, root_win, ExposureMask | ButtonPressMask | KeyPressMask | StructureNotifyMask);
	XSelectInput(display, addbtn_win, ExposureMask | ButtonPressMask | StructureNotifyMask);
	XMapWindow(display, root_win);
	XMapWindow(display, addbtn_win);
	// XRaiseWindow(display, addbtn_win);
	XFlush(display);
	
	/*
	Window child, child2;
	child = XCreateSimpleWindow(display, *root_win, 0, 0, 250, 75, 2, BlackPixel(display, screen_num), tilecolor.pixel);
	child2 = XCreateSimpleWindow(display, *root_win, 0, 75, 250, 75, 2, BlackPixel(display, screen_num), tilecolor.pixel);
	

	Pixmap shape_mask;
	shape_mask =  XCreateBitmapFromData(display, child, 
		tile_window_mask_250x75_bits, tile_window_mask_250x75_width, tile_window_mask_250x75_height); // depth - 1, i.e, Bitmap
	
	
	XShapeCombineMask(display, child, ShapeBounding, 0, 0, shape_mask, ShapeSet);
	XShapeCombineMask(display, child2, ShapeBounding, 0, 0, shape_mask, ShapeSet);
	
	XFreePixmap(display, shape_mask);

	GC gc;
	XGCValues values;
	XSelectInput(display, child, ExposureMask | ButtonPressMask | KeyPressMask);
	XMapWindow(display, child);
	XMapWindow(display, child2);
	*/
}

void start_event_loop(){
	XEvent report;
 	//XSetInputFocus(display, RootWindow(display, DefaultScreen(display)), RevertToParent, CurrentTime);
	while(1){
		XNextEvent(display, &report);
		switch(report.type){
		case Expose:
			
			if(report.xexpose.count!=0)
				break;
			
			if(NOTE_COUNT == 0){
				/* Display simple text msg */
				if(isInitialWindowMapped == 0){
					XMapWindow(display, initialmsg_window);
					XRaiseWindow(display, addbtn_win);
					isInitialWindowMapped = 1;
				}else{
					if(report.xexpose.window == initialmsg_window){
						drawInitialMsgString();
					}
				}
			}else{

			}
			if(report.xexpose.window == addbtn_win){
				drawAddNewBtn();
			}



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

void drawAddNewBtn(){
	XWindowAttributes btnwin_attr;
	if (XGetWindowAttributes(display, addbtn_win, &btnwin_attr) == 0) {
        fprintf(stderr, "Error getting window attributes\n");
        XCloseDisplay(display);
        exit(-1);
    }

	XGCValues values;
	unsigned long valuemask = 0; 
	GC gc = XCreateGC(display, addbtn_win, valuemask, &values);
	XSetForeground(display, gc, BlackPixel(display, screen_num));
	XSetBackground(display, gc, addnewbtncolor.pixel);
	Pixmap addbtn_icon = XCreateBitmapFromData(display, addbtn_win,addnewbtn_icon_bits, addnewbtn_icon_width, addnewbtn_icon_height);
	XCopyPlane(display, addbtn_icon, addbtn_win, gc, 0, 0, addnewbtn_icon_width, addnewbtn_icon_height, (btnwin_attr.width - addnewbtn_icon_height)/2 , (btnwin_attr.height - addnewbtn_icon_height)/2 , 1);
	XFreePixmap(display, addbtn_icon);
	XFreeGC(display, gc);
}

void createInitialMsgWindow(){
	int font_height = font_info->ascent + font_info->descent;

	XWindowAttributes rootwin_attr;
	if (XGetWindowAttributes(display, root_win, &rootwin_attr) == 0) {
        fprintf(stderr, "Error getting window attributes\n");
        XCloseDisplay(display);
        exit(-1);
    }

	int width = 250;
	int height = (font_height*2+50);
	int x = 0;
	int y = (rootwin_attr.height-height)/2;

	initialmsg_window = XCreateSimpleWindow(display, root_win, x,y,width,height,0,BlackPixel(display, screen_num), bgcolor.pixel);
	XSelectInput(display, initialmsg_window, ExposureMask);
	//XFlush(display);
}

void drawInitialMsgString(){
	char *initial_msg = "Empty. Try creating one.";

	GC gc;
	XGCValues values;
	unsigned long valuemask = 0; /* Ignoe XGCvalues and use defaults */
	gc = XCreateGC(display, initialmsg_window, valuemask, &values);

	XSetFont(display, gc, font_info->fid);
	XSetForeground(display, gc, BlackPixel(display, screen_num));

	draw_string_on_window(&initialmsg_window, &gc, -1, -1, initial_msg, strlen(initial_msg)); // Draw string on 5 offset from x, and center on y
	XFreeGC(display, gc);
}

void draw_string_on_window(Window *win, GC *gc, int x, int y, char *string, int strlength){
	/* IF x : -1, draw on x-center of window
	   IF y : -1, draw on y-center of window*/
	XWindowAttributes win_attr;
	XGetWindowAttributes(display, *win, &win_attr);

	int avilable_width = win_attr.width - x ;
	int text_width = XTextWidth(font_info, string, strlength);
	int max_chars_per_line = avilable_width / XTextWidth(font_info, "M", 1); // assuming "M" is widest charcter
	printf("max: %d\n", max_chars_per_line);
	char *buff = (char *)malloc(max_chars_per_line+1);
	int font_height = font_info->ascent + font_info->descent;

	int lines_needed = ((text_width % avilable_width) == 0)?text_width / avilable_width :(text_width / avilable_width) + 1;

	int x_offset =  x;
	int y_offset = y;

	if(x_offset == -1){
		if(lines_needed == 1)
			x_offset = (win_attr.width-text_width)/2;
		else{
			//avilable_width -= 50;
			x_offset = (win_attr.width - avilable_width)/2 ;
		}
	}
	if(y == -1)
		y_offset = (win_attr.height-(font_height*lines_needed+10))/2 + font_height;

	int _strlen = 0;

	// THIS FUNTION NEEDS TO BE REVISED

	for(int i=0;i<lines_needed;i++){
		int len = snprintf(buff, max_chars_per_line + 1, "%s", string + i * max_chars_per_line);
		printf("%s\n", buff);
		XDrawString(display, *win, *gc, x_offset, y_offset + i * (font_info->ascent + font_info->descent), buff, len);
	}

/*
	for(int i=0;i<lines_needed;i++){
		for(int j =0; j<strlength; j++){
			buff[j] = string[j + _strlen ];
			if((XTextWidth(font_info, buff, j+1) >= avilable_width) || string[j + _strlen + 1] == '\0'){
				
				if(x == -1){
					if(i+1 == lines_needed){
						x_offset = (win_attr.width - XTextWidth(font_info, buff, strlen(buff))) / 2;
					}
				}

				XDrawString(display, *win, *gc, x_offset, y_offset, buff, strlen(buff));
				y_offset += font_height;
				memset(buff, '\0', strlen(buff));
				_strlen += j + 1;
				break;
			}
		}	
	}
*/
	free(buff);
	//int x_offset = 5;
	//int y_offset = (win_attr.height - font_height  / 2);


}