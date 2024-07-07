#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <X11/Xft/Xft.h>

#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrender.h>

#include "main.h"
#include "utils.h"
#include "database.h"

#include "pixmaps/tile_window_mask_250x75.xbm"
#include "pixmaps/tile_window_mask_250x55.xbm"
#include "pixmaps/tile_window_mask_250x35.xbm"
#include "pixmaps/addnewbtn_window_mask.xbm"
#include "pixmaps/addnewbtn_icon.xbm"
#include "pixmaps/cross.xbm"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>
sqlite3 *db;


void open_display();
void alloc_colors();
void createAndMap_root_window(int argc, char **argv);
void start_event_loop();
void createInitialMsgWindow();
void drawInitialMsgString();
void draw_string_on_window(Window *win, XftColor color, int x_pad, int x, int y, char *string, int strlength);
void drawAddNewBtn();
void drawTodoInputExitBtn();
void drawStringInTodoTextfield(GC *textfeild_cursor_gc, char *string, int len);
void drawWordLimitIndicator(XftDraw *xft_draw, int len, int win_width, int win_height);
int calculate_average_char_width(XFontStruct *font_info);
void load_ttf_font();

Display *display;
int screen_num;
int display_width, display_height;
char *progname;


XFontStruct *font_info;

Window root_win, initialmsg_window, addbtn_win, todoinput_win, todoinputwin_exit_btn, todoinput_textfield, todoinput_submit_btn;

int isTODOListScrolling = 0;

TodoItem *datatable = NULL;
TodoItem *datatable_cursor = NULL;
TodoItem *datatable_firstitem = NULL;
TodoItem *datatable_lastitem = NULL;

FT_Library ft_library;

XftFont *xft_font_10, *xft_font_8; // font of size 10, 8
XftColor xft_color_black; // #000000FF
XftColor xft_color_tilecolor;
XftColor xft_color_wordindicator;

XColor bgcolor; // #333333
XColor tilecolor; // #B3D1C4
XColor addnewbtncolor;
XColor warningred;

Colormap colormap;
Visual *visual;

int isInitialWindowMapped = 0;
int warningColorSet = 0;
int db_primary_key;
int todo_count;
int todo_win_y_offset = 0;
int first_window_y_offset = 0;
int last_window_y_offset = 0;

void createTodoItem(TodoItem *cursor, char *data, char *date, int datalen, int id){
	// NEED TO REVIW
	int root_width = 250;
	int root_height = 150;

	cursor->next = (TodoItem *)malloc(sizeof(TodoItem));
	cursor->next->data = (char *)malloc(datalen+1);
	cursor->next->date = (char *)malloc(9);
	strcpy(cursor->next->data, data);
	strcpy(cursor->next->date, date);
	cursor->next->id = id;
	cursor->next->datalen = datalen;
	cursor->next->next = NULL;

	int avilable_width = (root_width - 15);
	int font_height = font_info->ascent + font_info->descent;
	int text_width = XTextWidth(font_info, data, datalen);
	int lines_needed = ((text_width % avilable_width) == 0)?text_width / avilable_width :(text_width / avilable_width) + 1;
	int currentHeight = (font_height*lines_needed)+font_height;

	XWindowAttributes win_attr;
	XGetWindowAttributes(display, cursor->win, &win_attr);

	int prevHeight = win_attr.y + win_attr.height + 5;
   	Window win = XCreateSimpleWindow(display, root_win, 5, prevHeight+5, (root_width - 15), currentHeight, 1, BlackPixel(display, screen_num), addnewbtncolor.pixel);
	XSelectInput(display, win, ExposureMask);
	XMapWindow(display, win);

	datatable_lastitem = cursor->next;
}

void freeTodoItem(TodoItem *item) {
    if (item) {
        if (item->data) {
            free(item->data);
        }
        if (item->date) {
            free(item->date);
        }
        XUnmapWindow(display, item->win);
        XDestroyWindow(display, item->win);
        free(item);
    }
}
void destroyTodoList(){
	if(datatable_firstitem!=NULL){		
	    TodoItem *current = datatable_firstitem;
	    TodoItem *next;

	    while (current != NULL) {
	        next = current->next;
	        freeTodoItem(current);
	        current = next;
	    }
	    datatable = NULL;
		datatable_cursor = NULL;
		datatable_firstitem = NULL;
		datatable_lastitem = NULL;
	}else{
		printf("NOTHING TO DESTROY\n");
	}
}

void populateTodoItems(){
	int root_width = 250;
	int root_height = 150;

	sqlite3_stmt *stmt;
	if( sqlite3_prepare_v2(db, "SELECT * FROM TODOTABLE", -1, &stmt, NULL) != SQLITE_OK){
		fprintf(stderr, "DB (error): Failed to fetch data: %s\n", sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		exit(-1);
	}

	int prevHeight = 0;
	int currentHeight = 0;

	int font_height = xft_font_10->ascent + xft_font_10->descent;
	int currentWidth = (root_width - 15);
	int avilable_width = (currentWidth - 30);

	TodoItem *prev = NULL;
	TodoItem *next = NULL;
	
	int max_chars_per_line = (avilable_width) / calculate_average_char_width(font_info);
	
	XGlyphInfo extents;
	for(int i=0; i<todo_count; i++){
		if(sqlite3_step(stmt) == SQLITE_ROW){
			char *data = (char *)sqlite3_column_text(stmt, 1);
			int length = strlen(data);
			XftTextExtentsUtf8(display, xft_font_10, (const FcChar8 *)data, length, &extents);
			int text_width = extents.xOff;
			int lines_needed = ((length % max_chars_per_line) == 0) ? length / max_chars_per_line : (length / max_chars_per_line) + 1;
			currentHeight = (font_height*lines_needed)+font_height;

		   	Window win = XCreateSimpleWindow(display, root_win, 5, prevHeight+5, currentWidth, currentHeight, 1, BlackPixel(display, screen_num), tilecolor.pixel);
			XSelectInput(display, win, ExposureMask | EnterWindowMask | LeaveWindowMask);
			XMapWindow(display, win);

			if(i == 0){ 
				datatable = (TodoItem *)malloc(sizeof(TodoItem));
				datatable_cursor = datatable;
				datatable_firstitem = datatable;
				first_window_y_offset = prevHeight+5;
			}
			datatable->win = win;
			datatable->id = sqlite3_column_int(stmt, 0);
			
			datatable->data = (char *)malloc(length+1);
			if(datatable->data) strcpy(datatable->data, data);

			datatable->datalen = length;

			datatable->date = (char *)malloc(9); 
			if(datatable->date) strcpy(datatable->date, sqlite3_column_text(stmt, 2));

			datatable->prev = prev;
			prev = datatable;
			
			if(i+1 != todo_count){
				next = (TodoItem *)malloc(sizeof(TodoItem));
			}else{
				next = NULL;
				datatable_lastitem = datatable;
			}
			datatable->next = next;
			datatable = datatable->next;
			prevHeight += currentHeight + 5;
		}
	}
	last_window_y_offset = prevHeight+5;
}

int main(int argc, char **argv){
	progname = argv[0];
	db_primary_key = prepare_database(&db);
	todo_count = fetch_todo_count(&db);
	open_display();

	// Loading font, to see list of fonts $ xlsfonts in terminal
	// to convert TTF to .h format: $ xxd -i SourceCodePro-Regular.ttf > source_code_pro.h
	// -b&h-lucida-medium-r-normal-sans-12-120-75-75-p-71-iso8859-1
	// BELOW IS BITMAPPED BASED FONT
	/*
	char *fontname = "-adobe-helvetica-medium-r-normal--14-140-75-75-p-77-iso8859-1";
	if((font_info = XLoadQueryFont(display, fontname)) == NULL ){
		printf("Error loading fonts\nexiting...\n");
		exit(-1);
	}
	*/

    visual = DefaultVisual(display, screen_num);
    colormap = DefaultColormap(display, screen_num);

    // CURRENTLY USING MONO SPACED FONT
    load_ttf_font();

	alloc_colors();
	createAndMap_root_window(argc, argv);
	createInitialMsgWindow();
	
	// MAP ALL WINDOWS ONCE, AND DRAW ONLY TO EXPOSED AREA
	populateTodoItems();

	start_event_loop();
	close_db(&db);

    XftColorFree(display, visual, colormap, &xft_color_black);
    XftFontClose(display, xft_font_10);
	XCloseDisplay(display);
	return 0;
}

void open_display(){
	// Open Display
	if((display = XOpenDisplay(NULL)) == NULL){
		fprintf(stderr, "Error Opening Display\nExiting..\n");
		exit(-1);
	}else{
		printf("%d\n", display);
		screen_num = DefaultScreen(display);
		// Get full display width and height
		display_width = DisplayWidth(display, screen_num);
		display_height = DisplayHeight(display, screen_num);
		printf("Successfully connect to display server\nDisplay Width: %d Display Height: %d\n", display_width, display_width);
	}
}

void alloc_colors(){
	// Allocate bgcolor
	if(!alloc_xcolor_from_hex(display, colormap, "#333333", &bgcolor)){
		fprintf(stderr, "Failed allocating BGColor\nexiting...\n");
		exit(-1);
	}
	if(!alloc_xcolor_from_hex(display, colormap, "#B3D1C4", &tilecolor)){
		fprintf(stderr, "Failed allocating BGColor\nexiting...\n");
		exit(-1);
	}
	if(!alloc_xcolor_from_hex(display, colormap, "#D0F0FF", &addnewbtncolor)){
		fprintf(stderr, "Failed allocating BGColor\nexiting...\n");
		exit(-1);
	}
	if(!alloc_xcolor_from_hex(display, colormap, "#FF0000", &warningred)){
		fprintf(stderr, "Failed allocating BGColor\nexiting...\n");
		exit(-1);
	}
	if(!alloc_xftcolor_from_hex(display, visual, colormap, "#000000FF", &xft_color_black)){
		fprintf(stderr, "Failed allocating BGColor\nexiting...\n");
		exit(-1);
	}
	if(!alloc_xftcolor_from_hex(display, visual, colormap, "#000000FF", &xft_color_wordindicator)){
		fprintf(stderr, "Failed allocating BGColor\nexiting...\n");
		exit(-1);
	}
	if(!alloc_xftcolor_from_hex(display, visual, colormap, "#C5C5C5FF", &xft_color_tilecolor)){
		fprintf(stderr, "Failed allocating BGColor\nexiting...\n");
		exit(-1);
	}
}

void load_ttf_font(){
 	if (!(xft_font_10 = XftFontOpen(display, screen_num, XFT_FAMILY, XftTypeString, "Source Code Pro", XFT_SIZE, XftTypeDouble, 10.0, NULL))) {
        fprintf(stderr, "Failed to load font\n");
        return;
    }
    if (!(xft_font_8 = XftFontOpen(display, screen_num, XFT_FAMILY, XftTypeString, "Source Code Pro", XFT_SIZE, XftTypeDouble, 8.0, NULL))) {
        fprintf(stderr, "Failed to load font\n");
        return;
    }
 	/*
 	XRenderColor xrcolor;
 	xrcolor.red = 0;
 	xrcolor.green = 0;
 	xrcolor.blue = 0;
 	xrcolor.alpha = 65535;
 	XftColorAllocValue(display, visual, colormap, &xrcolor, &xft_color);
 	*/
}

void createAndMap_root_window(int argc, char **argv){
	// Creating Root Window
	int root_x, root_y, root_width, root_height;
	root_width = 250;
	root_height = 150;
	root_x = display_width - root_width - 5;
	root_y = 8;
	root_win = XCreateSimpleWindow(display, RootWindow(display, screen_num), root_x, root_y, root_width, root_height, 2, BlackPixel(display, screen_num), bgcolor.pixel);
/*
	XRenderPictFormat* pict_format = XRenderFindStandardFormat(display, PictStandardARGB32);
	if (!pict_format) {
        fprintf(stderr, "Cannot find PictStandardARGB32 format\n");
        XCloseDisplay(display);
        exit(1);
    }

    XVisualInfo vinfo_template;
    vinfo_template.screen = screen_num;
    vinfo_template.depth = 32; // Looking for 32-bit depth visuals
    vinfo_template.class = TrueColor;
    int nitems;
    XVisualInfo* vinfo_list = XGetVisualInfo(display, VisualScreenMask | VisualDepthMask | VisualClassMask, &vinfo_template, &nitems);
    if (!vinfo_list) {
        fprintf(stderr, "No matching visual found\n");
        XCloseDisplay(display);
       	exit(1);
    }

    // Select a suitable visual from the list
    XVisualInfo* vinfo = NULL;
    for (int i = 0; i < nitems; i++) {
        if (XRenderFindVisualFormat(display, vinfo_list[i].visual) == pict_format) {
            vinfo = &vinfo_list[i];
            break;
        }
    }

    if (!vinfo) {
        fprintf(stderr, "Cannot find suitable visual\n");
        XFree(vinfo_list);
        XCloseDisplay(display);
        exit(1);
    }


    XSetWindowAttributes attrs;
    attrs.colormap = XCreateColormap(display, RootWindow(display, screen_num), vinfo->visual, AllocNone);
    attrs.border_pixel = 0;
    attrs.background_pixel = 0;
    root_win = XCreateWindow(display, RootWindow(display, screen_num), root_x, root_y, root_width, root_height, 0, vinfo->depth, InputOutput, vinfo->visual,
                                    CWColormap | CWBorderPixel | CWBackPixel, &attrs);

    int event_base, error_base;
    if (!XCompositeQueryExtension(display, &event_base, &error_base)) {
        fprintf(stderr, "Composite extension not available\n");
        XCloseDisplay(display);
        exit(1);
    }
    XCompositeRedirectWindow(display, root_win, CompositeRedirectAutomatic);

	Picture picture = XRenderCreatePicture(display, root_win, pict_format, 0, NULL);
	XRenderColor color;
    color.red = 0;
    color.green = 0;
    color.blue = 0;
    color.alpha = 0x7FFF;
	XRenderFillRectangle(display, PictOpSrc, picture, &color, 0, 0, root_width, root_height);
*/

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
	
	addbtn_win = XCreateSimpleWindow(display, root_win, btn_x,btn_y,btn_width,btn_height,0,BlackPixel(display, screen_num), addnewbtncolor.pixel);
	
	Pixmap addbtn_shapemask = XCreateBitmapFromData(display, addbtn_win, addnewbtn_window_mask_bits, addnewbtn_window_mask_width, addnewbtn_window_mask_height);

	XShapeCombineMask(display, addbtn_win, ShapeBounding, 0,0,addbtn_shapemask, ShapeSet);
	XFreePixmap(display, addbtn_shapemask);

	// Selecting Types of Input for Root Window
	
	XSelectInput(display, root_win, ExposureMask | ButtonPressMask);
	XSelectInput(display, addbtn_win, ExposureMask | ButtonPressMask);
	
	XMapWindow(display, root_win);
	XMapWindow(display, addbtn_win);

	todoinput_win = XCreateSimpleWindow(display, root_win, (root_width - (root_width -35) - 6)/2, (root_height - (root_height -35) - 6)/2, root_width - 35, root_height - 35, 3, BlackPixel(display, screen_num), addnewbtncolor.pixel);
	todoinputwin_exit_btn = XCreateSimpleWindow(display, todoinput_win, (root_width - 35) - 15, 5, 10, 10, 0, BlackPixel(display, screen_num), WhitePixel(display, screen_num));
	todoinput_textfield = XCreateSimpleWindow(display, todoinput_win, 0, 0, (root_width - 35), (root_height - 35)-25, 1, BlackPixel(display, screen_num), addnewbtncolor.pixel);
	
	XGlyphInfo extents;
	XftTextExtentsUtf8(display, xft_font_10, (const FcChar8 *)"Submit", 7, &extents);
	int str_submit_width = extents.xOff;
	todoinput_submit_btn = XCreateSimpleWindow(display, todoinput_win, ((root_width - 35) - (str_submit_width + 15))/2, (root_height - 35)-24, str_submit_width + 15, 25, 1, BlackPixel(display, screen_num), addnewbtncolor.pixel);

	XSelectInput(display, todoinput_win, ExposureMask | ButtonPressMask);
	XSelectInput(display, todoinputwin_exit_btn, ExposureMask | ButtonPressMask);
	XSelectInput(display, todoinput_textfield, ExposureMask | KeyPressMask);
	XSelectInput(display, todoinput_submit_btn, ExposureMask | ButtonPressMask | ButtonReleaseMask);
}

void start_event_loop(){
	XEvent report;
	char buffer[10] = "";
	int bufsize = 10;
	char string[100] = "";
	KeySym keysym;
	XComposeStatus compose;
	int count;
	int strlength = 0;
	
	XGlyphInfo str_submit_width_extents;
	XftTextExtentsUtf8(display, xft_font_10, (const FcChar8 *)"Submit", 6, &str_submit_width_extents);
	
	XGCValues values;
	GC defaultGC;
	defaultGC = XCreateGC(display, todoinput_textfield, 0, &values);
	XSetLineAttributes(display, defaultGC, 2, LineSolid, CapButt, JoinBevel);

	int font_height_10 =  xft_font_10->ascent + xft_font_10->descent;
	int SUBMIT_BTN_TEXT_X = ((str_submit_width_extents.xOff + 15) - str_submit_width_extents.xOff)/2;
	int SUBMIT_BTN_TEXT_Y = (25 - (font_height_10))/2 + xft_font_10->ascent;

	Window removeTileBtn = XCreateSimpleWindow(display, root_win, 0, 10, 12, 12, 0, BlackPixel(display, screen_num), tilecolor.pixel);
	XSelectInput(display, removeTileBtn, ExposureMask | ButtonPressMask);
	int removeTileBtn_pos = 0;

 	//XSetInputFocus(display, RootWindow(display, DefaultScreen(display)), RevertToParent, CurrentTime);
	while(1){
		XNextEvent(display, &report);
		
		/*if(!isTODOListScrolling){
			isTODOListScrolling = 0;
		}*/
		
		if(!todo_count)
			XUnmapWindow(display, removeTileBtn);

		switch(report.type){

		case Expose:
			if(report.xexpose.count!=0)
				break;
			if(report.xexpose.window == root_win){
				if(todo_count == 0){
					/* Display simple text msg */
					if(isInitialWindowMapped == 0){
						XMapWindow(display, initialmsg_window);
						drawInitialMsgString();
						isInitialWindowMapped = 1;
					}else{
						drawInitialMsgString();
					}
				}else{
					/*
					if(!isTODOListScrolling){}
					*/
				}
				XRaiseWindow(display, removeTileBtn);
				XRaiseWindow(display, addbtn_win);
				break;
			}
			if(report.xexpose.window == removeTileBtn){
				XDrawLine(display, removeTileBtn, defaultGC, 0, (12+1)/2, 12, (12+1)/2);
			}
			if(report.xexpose.window == addbtn_win){
				drawAddNewBtn();
				break;
			}
			if(report.xexpose.window == todoinput_win){
				drawTodoInputExitBtn();
				break;
			}
			if(report.xexpose.window == todoinput_textfield){
				drawStringInTodoTextfield(&defaultGC, string, strlength);
				XGrabKeyboard(display, todoinput_textfield, True, GrabModeAsync, GrabModeAsync, CurrentTime);
				//XGrabPointer(display, todoinput_win, True, ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
								//GrabModeAsync, GrabModeAsync, None, None, CurrentTime);				
				break;
			}
			if(report.xexpose.window == todoinput_submit_btn){
				XSetWindowBackground(display, todoinput_submit_btn, addnewbtncolor.pixel);
				XftDraw *xft_draw = XftDrawCreate(display, todoinput_submit_btn, visual, colormap);
				XftDrawStringUtf8(xft_draw, &xft_color_black, xft_font_10, SUBMIT_BTN_TEXT_X ,  SUBMIT_BTN_TEXT_Y, (XftChar8 *)"Submit", 6);
				XftDrawDestroy(xft_draw);
				break;
			}
			for(int i=0; i<todo_count; i++){
				if(report.xexpose.window == datatable_cursor->win){
					XClearWindow(display, report.xexpose.window);
					draw_string_on_window(&datatable_cursor->win, xft_color_black, 30, 5, -1, datatable_cursor->data, datatable_cursor->datalen);
				}
				datatable_cursor = datatable_cursor->next;
			}
			datatable_cursor = datatable_firstitem;;
			break;
		case EnterNotify:
			for(int i=0; i<todo_count; i++){
				if(report.xcrossing.window == datatable_cursor->win){
					XWindowAttributes win_attr;
					XGetWindowAttributes(display, datatable_cursor->win, &win_attr);
					removeTileBtn_pos = datatable_cursor->id;
					XMoveWindow(display, removeTileBtn, win_attr.width-20, win_attr.y + (win_attr.height)/2 - 12/2);
            		XMapWindow(display, removeTileBtn);
				}
				datatable_cursor = datatable_cursor->next;
			}
			datatable_cursor = datatable_firstitem;;
			break;
		/*case LeaveNotify:

			for(int i=0; i<todo_count; i++){
				if(report.xcrossing.window == datatable_cursor->win){
					if (report.xcrossing.detail == NotifyInferior) {
						printf("OHHKK\n");
						XWindowAttributes win_attr;
						XGetWindowAttributes(display, datatable_cursor->win, &win_attr);
						XUnmapWindow(display, removeTileBtn);
						// XDrawRectangle(display, report.xcrossing.window, defaultGC, (win_attr.width - 30), (win_attr.height - 2)/2 - 1, ((win_attr.width - 20) - (win_attr.width - 30)), 2);
						//XClearArea(display, datatable_cursor->win, (win_attr.width - 30), (win_attr.height - 2)/2 - 1, ((win_attr.width - 20) - (win_attr.width - 30)), 2, True);
					}
				}
				datatable_cursor = datatable_cursor->next;
			}
			datatable_cursor = datatable_firstitem;;
			break;*/
		case ButtonPress:
			if(report.xbutton.button == 1){		
				if(report.xbutton.window ==  removeTileBtn){
					XUnmapWindow(display, removeTileBtn);
					printf("REMOVED : %d\n", removeTileBtn_pos);
					destroyTodoList();
					db_delete_row(&db, removeTileBtn_pos);
					todo_count = fetch_todo_count(&db);
					db_primary_key = db_fetch_max_id(&db)+1;
					populateTodoItems();
				}
				if(report.xbutton.window == addbtn_win){
					XUnmapWindow(display, removeTileBtn);
					XMapWindow(display, todoinput_win);
					XMapWindow(display, todoinputwin_exit_btn);
					XMapWindow(display, todoinput_textfield);
					XMapWindow(display, todoinput_submit_btn);

					XRaiseWindow(display, todoinput_win);
					XRaiseWindow(display, todoinput_textfield);
					XRaiseWindow(display, todoinputwin_exit_btn);
					XRaiseWindow(display, todoinput_submit_btn);
					break;
				}
				if(report.xbutton.window == todoinputwin_exit_btn){	
					XUngrabKeyboard(display, CurrentTime);
					XUnmapWindow(display, todoinput_win);
					break;
				}
				if(report.xbutton.window == todoinput_submit_btn){
					if(strlength){
						XSetWindowBackground(display, todoinput_submit_btn, WhitePixel(display, screen_num));
						XClearWindow(display, todoinput_submit_btn);
						XftDraw *xft_draw = XftDrawCreate(display, todoinput_submit_btn, visual, colormap);
	    				XftDrawStringUtf8(xft_draw, &xft_color_black, xft_font_10, SUBMIT_BTN_TEXT_X, SUBMIT_BTN_TEXT_Y, (XftChar8 *)"Submit", 6);
						XftDrawDestroy(xft_draw);
					}
					break;
				}

	/*			for(int i=0; i<todo_count; i++){
					if(report.xbutton.window == datatable_cursor->win){
						XWindowAttributes win_attr;
						XGetWindowAttributes(display, datatable_cursor->win, &win_attr);
						if(((report.xbutton.x > (win_attr.width - 35)) && (report.xbutton.x < (win_attr.width - 10))) && ((report.xbutton.y > (win_attr.height - 2)/2 - 10) && (report.xbutton.y < (win_attr.height - 2)/2 + 10)) ){
							printf("REMOVE : %d\n", i);
						}
					}
					datatable_cursor = datatable_cursor->next;
				}
				datatable_cursor = datatable_firstitem;*/
				break;
			}
			if(report.xbutton.window == root_win){
				XUnmapWindow(display, removeTileBtn);
				if(report.xbutton.button == 4){
				//XUnmapWindow(display, removeTileBtn);
					// SCROLL UP EVEN
					XWindowAttributes win_attr;
					XWindowAttributes last_win_attr;
					XGetWindowAttributes(display, datatable_lastitem->win, &last_win_attr);
					//printf("%d---\n", last_win_attr.y);
					//if(last_win_attr.y > 145){
						todo_win_y_offset += 1;
						for(int i=0; i<todo_count; i++){
							XGetWindowAttributes(display, datatable_cursor->win, &win_attr);
							XMoveWindow(display, datatable_cursor->win, 5, win_attr.y + todo_win_y_offset);
							datatable_cursor = datatable_cursor->next;
						}
						datatable_cursor = datatable_firstitem;
					//}
				}
				if(report.xbutton.button == 5){
					// SCROLL DOWN EVENT
					XWindowAttributes win_attr;
					XWindowAttributes first_win_attr;
					XGetWindowAttributes(display, datatable_firstitem->win, &first_win_attr);
					//if(first_win_attr.y > first_window_y_offset){
						todo_win_y_offset += 1;
						for(int i=0; i<todo_count; i++){
							XGetWindowAttributes(display, datatable_cursor->win, &win_attr);
							XMoveWindow(display, datatable_cursor->win, 5, win_attr.y - todo_win_y_offset);
							datatable_cursor = datatable_cursor->next;
						}	
						datatable_cursor = datatable_firstitem;;
					//}
				}
				break;
			}
			break;
		case ButtonRelease:
			if(report.xbutton.button == 1){
				if(report.xbutton.window == todoinput_submit_btn){
					if(strlength){
						XSetWindowBackground(display, todoinput_submit_btn, addnewbtncolor.pixel);
						XClearWindow(display, todoinput_submit_btn);
					    XftDraw *xft_draw = XftDrawCreate(display, todoinput_submit_btn, visual, colormap);
						XftDrawStringUtf8(xft_draw, &xft_color_black, xft_font_10, SUBMIT_BTN_TEXT_X, SUBMIT_BTN_TEXT_Y, (XftChar8 *)"Submit", 6);
						XftDrawDestroy(xft_draw);

						// LATER HANDLE THE DATA INSERTION IN PARALLEL TO IMPROVE PERFORMANCE
						if(!insert_into_todotable(&db, db_primary_key,string)) exit(0);
						db_primary_key++;
						todo_count = fetch_todo_count(&db);
						memset(string, 0, strlength);
						strlength = 0;
						destroyTodoList();
						populateTodoItems();
						XUngrabKeyboard(display, CurrentTime);
						XUnmapWindow(display, todoinput_win);
					}else{
						printf("Nothing to SUBMIT!\n");
					}
					break;
				}
			}
			break;
		case KeyPress:
			if(report.xkey.window == todoinput_textfield){
				count = XLookupString(&(report.xkey), buffer, bufsize, &keysym, &compose);
				if((keysym == XK_Return) || (keysym == XK_KP_Enter) || (keysym == XK_Linefeed)){
					// HANDLE \N LATER
					//strcat(string, "\n");
					// strcat(string, "\0");
					
				}
				else if(((keysym >= XK_KP_Space) && (keysym <= XK_KP_9)) || ((keysym>=XK_space) && (keysym <= XK_asciitilde))){
					if(strlength< 100) strcat(string, buffer); else printf("MAX CHAR REACHED\n");
					// strcat(string, "\0");
					//printf("%s\n", string);
					
				}
				else if((keysym == XK_BackSpace) || (keysym == XK_Delete)){
					if(strlength > 0){
						string[strlength-1] = '\0';
						XClearWindow(display, todoinput_textfield);
					}
				}
				strlength = strlen(string);
				drawStringInTodoTextfield(&defaultGC ,string, strlength);
			}
			break;
		default:
			break;
		}
		fflush(stdout);
		XFlush(display);
	}
	XFreeGC(display, defaultGC);
}



void drawStringInTodoTextfield(GC *textfeild_cursor_gc, char *string, int length){
	XClearWindow(display, todoinput_textfield);
	XWindowAttributes win_attr;
	if (XGetWindowAttributes(display, todoinput_textfield, &win_attr) == 0) {
        fprintf(stderr, "Error getting window attributes\n");
        XCloseDisplay(display);
        exit(-1);
    }
    
 	int font_height = xft_font_10->ascent + xft_font_10->descent;
	int max_chars_per_line = (win_attr.width - 30) / calculate_average_char_width(font_info);
	int lines_needed;
	if(length!=0)
		lines_needed = ((length % max_chars_per_line) == 0) ? length / max_chars_per_line : (length / max_chars_per_line) + 1;
	else lines_needed = 1;
	
	int x = 10;
	int y = 15;

	char *buff = (char *)malloc(max_chars_per_line+1);
    XftDraw *xft_draw = XftDrawCreate(display, todoinput_textfield, visual, colormap);
	for(int i=0;i<lines_needed;i++){
		int _len = snprintf(buff, max_chars_per_line+1, "%s", string + i * max_chars_per_line);
		XftDrawStringUtf8(xft_draw, &xft_color_black, xft_font_10, x, y + i * font_height, (XftChar8 *)buff, strlen(buff));
	}

	if(length==100){
		if(!alloc_xftcolor_from_hex(display, visual, colormap, "#FF0000FF", &xft_color_wordindicator)){
			fprintf(stderr, "Failed allocating BGColor\nexiting...\n");
			exit(-1);
		}
		warningColorSet = 1;
	}

	else if(warningColorSet==1){
		if(!alloc_xftcolor_from_hex(display, visual, colormap, "#000000FF", &xft_color_wordindicator)){
			fprintf(stderr, "Failed allocating BGColor\nexiting...\n");
			exit(-1);
		}
	} 


	drawWordLimitIndicator(xft_draw, length, win_attr.width, win_attr.height);
	XftDrawDestroy(xft_draw);

	// Drawing Cursor in textfield
	XGCValues values;
	XGlyphInfo buf_extents;
	XftTextExtentsUtf8(display, xft_font_10, (const FcChar8 *)buff, strlen(buff), &buf_extents);

	int buff_width = buf_extents.xOff;
	int cursor_x1 = x + buff_width + 5;
	int cursor_y2 = y + (lines_needed - 1)*font_height + 2;
	int cursor_x2 = cursor_x1;
	int cursor_y1 = y + (lines_needed - 2)*font_height + 5;

	XDrawLine(display, todoinput_textfield, *textfeild_cursor_gc, cursor_x1, cursor_y1, cursor_x2, cursor_y2);
	free(buff);
}

void drawWordLimitIndicator(XftDraw *xft_draw, int len, int win_width, int win_height){
	char int_buf[8];
	sprintf(int_buf, "%d", len);
	char buf[5]="/100";
	strcat(int_buf, buf);
	XGlyphInfo extents;
	XftTextExtentsUtf8(display, xft_font_8, (const FcChar8 *)"100/100", 7, &extents);
	XftDrawStringUtf8(xft_draw, &xft_color_wordindicator, xft_font_8, win_width - extents.xOff - 10, win_height - 5, (XftChar8 *)int_buf, strlen(int_buf));
}

void drawTodoInputExitBtn(){
	XGCValues values;
	unsigned long valuemask = 0; 
	GC gc = XCreateGC(display, todoinputwin_exit_btn, valuemask, &values);
	XSetForeground(display, gc, BlackPixel(display, screen_num));
	XSetBackground(display, gc, addnewbtncolor.pixel);
	Pixmap exitbtn_icon = XCreateBitmapFromData(display, todoinputwin_exit_btn, cross_bits, cross_width, cross_height);
	XCopyPlane(display, exitbtn_icon, todoinputwin_exit_btn, gc, 0, 0, cross_width, cross_height, 0 , 0 , 1);
	XFreePixmap(display, exitbtn_icon);
	XFreeGC(display, gc);
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
	int font_height = xft_font_10->ascent + xft_font_10->descent;

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
}

void drawInitialMsgString(){
	char *initial_msg = "Empty. Try creating one.";
	draw_string_on_window(&initialmsg_window, xft_color_tilecolor, 0,-1, -1, initial_msg, strlen(initial_msg));
}

int calculate_average_char_width(XFontStruct *font_info) {
    char sample_text[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int sample_text_length = strlen(sample_text);
    XGlyphInfo extents;
	XftTextExtentsUtf8(display, xft_font_10, (const FcChar8 *)sample_text, sample_text_length, &extents);
    int sample_text_width = extents.xOff;
    return sample_text_width / sample_text_length;
}

void draw_string_on_window(Window *win, XftColor color, int x_pad, int x, int y, char *string, int strlength){
	/* IF x : -1, draw on x-center of window
	   IF y : -1, draw on y-center of window*/
	XWindowAttributes win_attr;
	XGetWindowAttributes(display, *win, &win_attr);

	int avilable_width = win_attr.width - x - 15 - x_pad;
	
	XGlyphInfo extents;
	XftTextExtentsUtf8(display, xft_font_10, (const FcChar8 *)string, strlen(string), &extents);
	int text_width = extents.xOff;
	int max_chars_per_line = (avilable_width) / calculate_average_char_width(font_info); // assuming "M" is widest charcter
 	int font_height = xft_font_10->ascent + xft_font_10->descent;
	int lines_needed = ((strlength % max_chars_per_line) == 0) ? strlength / max_chars_per_line : (strlength / max_chars_per_line) + 1;
	char *buff = (char *)malloc(max_chars_per_line+1);

	int x_offset =  x;
	int y_offset = y;

	if(x_offset == -1){
		if(lines_needed == 1)
			x_offset = (win_attr.width-text_width)/2;
		else{
			x_offset = (win_attr.width - avilable_width)/2 ;
		}
	}
	if(y == -1)
		y_offset = (win_attr.height-(font_height*lines_needed))/2 + xft_font_10->ascent;
 	
	
 	
 	XftDraw *xft_draw = XftDrawCreate(display, *win, visual, colormap);
	for(int i=0;i<lines_needed;i++){
		snprintf(buff, max_chars_per_line + 1, "%s", string + i * max_chars_per_line);
    	XftDrawStringUtf8(xft_draw, &color, xft_font_10, x_offset, y_offset + i * font_height, (XftChar8 *)buff, strlen(buff));
		// XDrawString(display, *win, *gc, x_offset, y_offset + i * (font_info->ascent + font_info->descent), buff, strlen(buff));
	}
	
/*
	XGlyphInfo extents;
    const char *start = string;
    const char *end;
    int line_height = xft_font_10->ascent + xft_font_10->descent;

    while (*start) {
        // Find the maximum substring that fits within the window width
        end = start;
        while (*end && XftTextExtentsUtf8(NULL, xft_font_10, (XftChar8 *)start, end - start + 1, &extents) , extents.width <= avilable_width) {
            end++;
        }

        // Draw the substring that fits
        XftDrawStringUtf8(xft_draw, &xft_color_black, xft_font_10, x, y, (XftChar8 *)start, end - start);

        // Move to the next line
        start = end;
        y += line_height;
    }
*/

	// THE BELOW METHOD CALCULATE THE WIDTH OF EVERY CHAR IN BUFFER
	// THUS BELOW METHOD BETTER TO BE USE WHEN WE ARE WORKING ON
	// FONTS WITH NOT-FIXED WIDTH
/*
	int _strlen = 0;
	printf("-->%d\n", strlength);
	for(int i=0;i<lines_needed;i++){
		for(int j =0; j<strlength; j++){
			buff[j] = string[j + _strlen ];
			XGlyphInfo buf_extents_0;
			XftTextExtentsUtf8(display, xft_font_10, (const FcChar8 *)buff, j+1, &buf_extents_0);
			int buf_extents_0_width = extents.xOff;

			///if((XTextWidth(font_info, buff, j+1) >= avilable_width) || string[j + _strlen + 1] == '\0'){
			if((buf_extents_0_width >= avilable_width) || string[j + _strlen + 1] == '\0'){
				if(x == -1){
					if(i+1 == lines_needed){
						XGlyphInfo buf_extents_1;
						XftTextExtentsUtf8(display, xft_font_10, (const FcChar8 *)buff, strlen(buff), &buf_extents_1);
						int buf_extents_1_width = extents.xOff;

						x_offset = (win_attr.width - buf_extents_1_width) / 2;
						//x_offset = (win_attr.width - XTextWidth(font_info, buff, strlen(buff))) / 2;
					}
				}

				XftDrawStringUtf8(xft_draw, &xft_color_black, xft_font_10, x_offset, y_offset, (XftChar8 *)buff, strlen(buff));
				//XDrawString(display, *win, *gc, x_offset, y_offset, buff, strlen(buff));
				y_offset += font_height;
				memset(buff, '\0', strlen(buff));
				_strlen += j + 1;
				break;
			}
		}	
	}
*/
	free(buff);
	XftDrawDestroy(xft_draw);
}