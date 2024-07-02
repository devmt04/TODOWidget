#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>

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
void draw_string_on_window(Window *win, GC *gc, int x, int y, char *string, int strlength);
void drawAddNewBtn();
void drawTodoInputExitBtn();
void drawStringInTodoTextfield(GC *textGC, GC *wordLimitIndicator_gc, char *string, int len);
void drawWordLimitIndicator(GC *gc,int len, int win_width, int win_height);
int calculate_average_char_width(XFontStruct *font_info);
Display *display;
int screen_num;
int display_width, display_height;
char *progname;
XColor bgcolor; // #747474
XColor tilecolor; // #C8B4B4
XColor addnewbtncolor;
XColor warningred;

XFontStruct *font_info;

Window root_win, initialmsg_window, addbtn_win, todoinput_win, todoinputwin_exit_btn, todoinput_textfield, todoinput_submit_btn;
int isTODOListScrolling = 0;

TodoItem *datatable = NULL;
TodoItem *datatable_cursor;
TodoItem *datatable_firstitem;
/*
typedef struct {
	Window *windows;
	int size;
	int capacity;
} TODOWinArray;

typedef struct {
	char *data;
	int size;
} TODOData;
*/

int isInitialWindowMapped = 0;
int warningColorSet = 0;
int db_primary_key;
int todo_count;
int todo_win_y_offset = 0;
// TODOWinArray todoWinArr;
/*
void initTODOWinArray(TODOWinArray *arr, int initialCapacity){
	//datatable = (TODODataTable *)malloc(sizeof(TODODataTable));
	arr->windows = (Window *)malloc(initialCapacity * sizeof(Window));
	arr->size = 0;
	arr->capacity = initialCapacity;
	
}

void addInTODOWinArray(TODOWinArray *arr, Window win){
	if(arr->size == arr->capacity){
		arr->capacity *= 2;
		arr->windows = (Window *)realloc(arr->windows, arr->capacity * sizeof(Window));
	}
	arr->windows[arr->size++] = win;
}

void freeWindowArray(TODOWinArray *arr) {
    free(arr->windows);
    arr->windows = NULL;
    arr->size = 0;
    arr->capacity = 0;
}
*/
int loadDatabase(){
	int size = 0;
	sqlite3_stmt *stmt;
	if( sqlite3_prepare_v2(db, "SELECT * FROM TODOTABLE", -1, &stmt, NULL) != SQLITE_OK){
		fprintf(stderr, "DB (error): Failed to fetch data: %s\n", sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		exit(-1);
	}

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		char *data = (char *)sqlite3_column_text(stmt, 1);
		int length = strlen(data);


	}

	return size;
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

	int font_height = font_info->ascent + font_info->descent;
	int avilable_width = (root_width - 15);

	TodoItem *prev = NULL;
	TodoItem *next = NULL;

	for(int i=0; i<todo_count; i++){
		if(sqlite3_step(stmt) == SQLITE_ROW){
			char *data = (char *)sqlite3_column_text(stmt, 1);
			int length = strlen(data);
			
			int text_width = XTextWidth(font_info, data, length);
			int lines_needed = ((text_width % avilable_width) == 0)?text_width / avilable_width :(text_width / avilable_width) + 1;
			currentHeight = (font_height*lines_needed)+font_height;

		   	Window win = XCreateSimpleWindow(display, root_win, 5, prevHeight+5, (root_width - 15), currentHeight, 1, BlackPixel(display, screen_num), addnewbtncolor.pixel);
			XSelectInput(display, win, ExposureMask);
			XMapWindow(display, win);

			if(i == 0){ 
				datatable = (TodoItem *)malloc(sizeof(TodoItem));
				datatable_cursor = datatable;
				datatable_firstitem = datatable;
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
			}
			datatable->next = next;
			datatable = datatable->next;
			prevHeight += currentHeight + 5;

		}
	}


/*
	//
 	printf("%d | %s | %s \n", 
	           sqlite3_column_int(stmt, 0), 
	           sqlite3_column_text(stmt, 1), 
	           sqlite3_column_text(stmt, 2));

		//GC gc;
	//gc = XCreateGC(display, root_win, 0, NULL);
	//XSetFont(display, gc, font_info->fid);

	//int font_height = font_info->ascent + font_info->descent;
	//int avilable_width = (root_width - 15) ;

	//char *data = (char *)sqlite3_column_text(stmt, 1);
	//int length = strlen(data);

	//int text_width = XTextWidth(font_info, data, length);
	//int lines_needed = ((text_width % avilable_width) == 0)?text_width / avilable_width :(text_width / avilable_width) + 1;
	
	//currentHeight = (font_height*lines_needed)+font_height;
	//draw_string_on_window(&win, &gc, 5, -1, data, length);

	//addInTODOWinArray(&todoWinArr, win);
	//XFreeGC(display, gc);
	//sqlite3_finalize(stmt);
	*/
}

int main(int argc, char **argv){
	progname = argv[0];
	db_primary_key = prepare_database(&db);

	todo_count = fetch_todo_count(&db);

	

	//initTODOWinArray(&todoWinArr, 5);
	open_display();

	// Loading font, to see list of fonts $ xlsfonts in terminal
	
	char *fontname = "-b&h-lucida-medium-r-normal-sans-12-120-75-75-p-71-iso8859-1";
	if((font_info = XLoadQueryFont(display, fontname)) == NULL ){
		printf("Error loading fonts\nexiting...\n");
		exit(-1);
	}

	alloc_colors();
	createAndMap_root_window(argc, argv);
	createInitialMsgWindow();
	
	// MAP ALL WINDOWS ONCE, AND DRAW ONLY TO EXPOSED AREA
	populateTodoItems();

	start_event_loop();
	close_db(&db);
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
	if(!alloc_color_from_hex(display, colormap, "#FF0000", &warningred)){
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
	
	addbtn_win = XCreateSimpleWindow(display, root_win, btn_x,btn_y,btn_width,btn_height,0,BlackPixel(display, screen_num), addnewbtncolor.pixel);
	
	Pixmap addbtn_shapemask = XCreateBitmapFromData(display, addbtn_win, addnewbtn_window_mask_bits, addnewbtn_window_mask_width, addnewbtn_window_mask_height);

	XShapeCombineMask(display, addbtn_win, ShapeBounding, 0,0,addbtn_shapemask, ShapeSet);
	XFreePixmap(display, addbtn_shapemask);

	// Selecting Types of Input for Root Window
	
	XSelectInput(display, root_win, ExposureMask | ButtonPressMask);
	XSelectInput(display, addbtn_win, ExposureMask | ButtonPressMask);
	
	XMapWindow(display, root_win);

	XMapWindow(display, addbtn_win);

	todoinput_win = XCreateSimpleWindow(display, root_win, (root_width - (root_width -35))/2, (root_height - (root_height -35))/2, root_width - 35, root_height - 35, 3, BlackPixel(display, screen_num), addnewbtncolor.pixel);
	todoinputwin_exit_btn = XCreateSimpleWindow(display, todoinput_win, (root_width - 35) - 15, 5, 10, 10, 0, BlackPixel(display, screen_num), WhitePixel(display, screen_num));
	todoinput_textfield = XCreateSimpleWindow(display, todoinput_win, 0, 0, (root_width - 35), (root_height - 35)-25, 1, BlackPixel(display, screen_num), addnewbtncolor.pixel);
	todoinput_submit_btn = XCreateSimpleWindow(display, todoinput_win, ((root_width - 35) - (XTextWidth(font_info, "Submit", 6) + 15))/2, (root_height - 35)-24, XTextWidth(font_info, "Submit", 6) + 15, 25, 1, BlackPixel(display, screen_num), addnewbtncolor.pixel);
	// SHOW THIS POPUP AS A NOTICE

	XSelectInput(display, todoinput_win, ExposureMask | ButtonPressMask);
	XSelectInput(display, todoinputwin_exit_btn, ExposureMask | ButtonPressMask);
	XSelectInput(display, todoinput_textfield, ExposureMask | KeyPressMask);
	XSelectInput(display, todoinput_submit_btn, ExposureMask | ButtonPressMask | ButtonReleaseMask);
	//XFlush(display);
	
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
	char buffer[10] = "";
	int bufsize = 10;
	char string[100] = "";
	KeySym keysym;
	XComposeStatus compose;
	int count;
	int strlength = 0;
	GC submitbtn_gc, textGC, wordLimitIndicator_gc;

 	//XSetInputFocus(display, RootWindow(display, DefaultScreen(display)), RevertToParent, CurrentTime);
	while(1){
		XNextEvent(display, &report);

		if(!isTODOListScrolling){
			isTODOListScrolling = 0;
		}

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
					if(!isTODOListScrolling){
					}
					*/

				}
				XRaiseWindow(display, addbtn_win);
			}
			if(report.xexpose.window == addbtn_win){
				drawAddNewBtn();
			}
			if(report.xexpose.window == todoinput_win){
				drawTodoInputExitBtn();
			}
			if(report.xexpose.window == todoinput_textfield){
				drawStringInTodoTextfield(&textGC, &wordLimitIndicator_gc, string, strlength);
			}
			// for(int i=0; i<todoWinArr.size;i++){
			// 	if(report.xexpose.window == todoWinArr.windows[i]){
			// 		//GC gc;
			// 		//gc = XCreateGC(display, root_win, 0, NULL);
			// 		//XSetFont(display, gc, font_info->fid);

			// 		sqlite3_stmt *stmt;
			// 		if( sqlite3_prepare_v2(db, "SELECT * FROM TODOTABLE", -1, &stmt, NULL) != SQLITE_OK){
			// 			fprintf(stderr, "DB (error): Failed to fetch data: %s\n", sqlite3_errmsg(db));
			// 			sqlite3_finalize(stmt);
			// 			exit(-1);
			// 		}

			// 		char *data = (char *)sqlite3_column_text(stmt, 1);
			// 		int length = strlen(data);

			// 		draw_string_on_window(&todoWinArr.windows[i], &textGC, 5, -1, data, length);
			// 		//XFreeGC();
			// 	}

			// }
			break;
		case ButtonPress:
			if(report.xbutton.window == addbtn_win){
				// XGCValues values;
				unsigned long valuemask = 0; 
				textGC = XCreateGC(display, todoinput_textfield, valuemask, NULL);

				submitbtn_gc = XCreateGC(display, todoinput_submit_btn, (long)0, NULL);
				wordLimitIndicator_gc =  XCreateGC(display, todoinput_textfield, (long)0, NULL);
				XSetFont(display, wordLimitIndicator_gc, font_info->fid);
				XSetFont(display, submitbtn_gc, font_info->fid);
				XSetFont(display, textGC, font_info->fid);

				XMapWindow(display, todoinput_win);
				XMapWindow(display, todoinputwin_exit_btn);
				XMapWindow(display, todoinput_textfield);
				XMapWindow(display, todoinput_submit_btn);

				XRaiseWindow(display, todoinput_win);
				XRaiseWindow(display, todoinput_textfield);
				XRaiseWindow(display, todoinputwin_exit_btn);
				XRaiseWindow(display, todoinput_submit_btn);
			}
			if(report.xbutton.window == todoinputwin_exit_btn){	
				XUnmapWindow(display, todoinput_win);
				XFreeGC(display, textGC);
				XFreeGC(display, submitbtn_gc);
				XFreeGC(display, wordLimitIndicator_gc);
			}
			if(report.xbutton.window == todoinput_submit_btn){
				if(strlength){
					XSetWindowBackground(display, todoinput_submit_btn, WhitePixel(display, screen_num));
					XClearWindow(display, todoinput_submit_btn);
					XDrawString(display, todoinput_submit_btn, submitbtn_gc, ((XTextWidth(font_info, "Submit", 6) + 15) - XTextWidth(font_info, "Submit", 6))/2, (25 - (font_info->ascent + font_info->descent))/2 + font_info->ascent, "Submit", 6);
				}
			}
			if(report.xbutton.window == root_win){
				
				XWindowAttributes win_attr;
				if(report.xbutton.button == 4){
					// SCROLL UP EVENT
					isTODOListScrolling = 1;
					todo_win_y_offset += 2;
					for(int i=0; i<todo_count; i++){
						XGetWindowAttributes(display, datatable_cursor->win, &win_attr);
						XMoveWindow(display, datatable_cursor->win, 5, win_attr.y + todo_win_y_offset);
						datatable_cursor = datatable_cursor->next;
					}
					datatable_cursor = datatable_firstitem;;
				}

				if(report.xbutton.button == 5){
					// SCROLL DOWN EVENT
					isTODOListScrolling = 1;
					todo_win_y_offset += 2;
					for(int i=0; i<todo_count; i++){
						XGetWindowAttributes(display, datatable_cursor->win, &win_attr);
						XMoveWindow(display, datatable_cursor->win, 5, win_attr.y - todo_win_y_offset);
						datatable_cursor = datatable_cursor->next;
					}
					datatable_cursor = datatable_firstitem;;
				}
			}
			break;
		case ButtonRelease:
			if(report.xbutton.window == todoinput_submit_btn){
				if(strlength){
					XSetWindowBackground(display, todoinput_submit_btn, addnewbtncolor.pixel);
					XClearWindow(display, todoinput_submit_btn);
					// XTextWidth(font_info, "Submit", 6) + 15   25
					XDrawString(display, todoinput_submit_btn, submitbtn_gc, ((XTextWidth(font_info, "Submit", 6) + 15) - XTextWidth(font_info, "Submit", 6))/2, (25 - (font_info->ascent + font_info->descent))/2 + font_info->ascent, "Submit", 6);

					// LATER HANDLE THE DATA INSERTION IN PARALLEL TO IMPROVE PERFORMANCE
					if(!insert_into_todotable(&db, db_primary_key,string)) exit(0);
					
					db_primary_key++;
					todo_count = fetch_todo_count(&db);

					memset(string, 0, sizeof(string));
					strlength = 0;
					XUnmapWindow(display, todoinput_win);
					XFreeGC(display, textGC);
					XFreeGC(display, submitbtn_gc);
					XFreeGC(display, wordLimitIndicator_gc);
				}else{
					printf("Nothing to SUBMIT!\n");
				}
			}
		case KeyPress:
			if(report.xkey.window == todoinput_textfield){
				//printf("%d\n", strlen(string));
				count = XLookupString(&(report.xkey), buffer, bufsize, &keysym, &compose);
				//printf("ASCII ???? value: %d\n", count);
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
				drawStringInTodoTextfield(&textGC, &wordLimitIndicator_gc, string, strlength);
			}
			break;
		default:
			break;
		}
		fflush(stdout);
		XFlush(display);
	}
}



void drawStringInTodoTextfield(GC *textGC, GC *wordLimitIndicator_gc, char *string, int length){
	XClearWindow(display, todoinput_textfield);
	XWindowAttributes win_attr;
	if (XGetWindowAttributes(display, todoinput_textfield, &win_attr) == 0) {
        fprintf(stderr, "Error getting window attributes\n");
        XCloseDisplay(display);
        exit(-1);
    }

	//int text_width = XTextWidth(font_info, string, len);
	// int lines_needed = ((text_width % win_attr.width) == 0) ? text_width / win_attr.width : (text_width / win_attr.width) + 1;
	int font_height = font_info->ascent + font_info->descent;
	int max_chars_per_line = win_attr.width / XTextWidth(font_info, "M", 1);

	int lines_needed = ((length % max_chars_per_line) == 0) ? length / max_chars_per_line : (length / max_chars_per_line) + 1;
	
	int x = 10;
	int y = 15;

	char *buff = (char *)malloc(max_chars_per_line+1);
	for(int i=0;i<lines_needed;i++){
		int _len = snprintf(buff, max_chars_per_line+1, "%s", string + i * max_chars_per_line);
		XDrawString(display, todoinput_textfield, *textGC, x, y + i * font_height, buff, strlen(buff));
	}

	if(length==100){
		XSetForeground(display, *wordLimitIndicator_gc, warningred.pixel);
		warningColorSet = 1;
	}
	else if(warningColorSet==1) XSetForeground(display, *wordLimitIndicator_gc, BlackPixel(display, screen_num));
	
	drawWordLimitIndicator(wordLimitIndicator_gc, length, win_attr.width, win_attr.height);

	//
	/*
	int buff_width = XTextWidth(font_info, buff, strlen(buff));
	printf("%d\n", buff_width);
	int cursor_x1 = x + buff_width;
	int cursor_y1 = y + (lines_needed)*font_height;
	int cursor_x2 = cursor_x1;
	int cursor_y2 = y + (lines_needed-1)*font_height;
	XDrawLine(display, todoinput_textfield, *gc, cursor_x1, cursor_y1, cursor_x2, cursor_y2);
	*/
	free(buff);
}
void drawWordLimitIndicator(GC *gc,int len, int win_width, int win_height){
	char int_buf[8];
	sprintf(int_buf, "%d", len);
	char buf[5]="/100";
	strcat(int_buf, buf);
	XDrawString(display, todoinput_textfield, *gc, win_width- XTextWidth(font_info, "100/100", 7) , win_height - 5, int_buf, strlen(int_buf));
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

int calculate_average_char_width(XFontStruct *font_info) {
    char sample_text[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int sample_text_length = strlen(sample_text);
    int sample_text_width = XTextWidth(font_info, sample_text, sample_text_length);
    return sample_text_width / sample_text_length;
}

void draw_string_on_window(Window *win, GC *gc, int x, int y, char *string, int strlength){
	/* IF x : -1, draw on x-center of window
	   IF y : -1, draw on y-center of window*/
	XWindowAttributes win_attr;
	XGetWindowAttributes(display, *win, &win_attr);

	int avilable_width = win_attr.width - x ;
	
	int text_width = XTextWidth(font_info, string, strlength);
	int max_chars_per_line = avilable_width / calculate_average_char_width(font_info); // assuming "M" is widest charcter
	int font_height = font_info->ascent + font_info->descent;
	int lines_needed = ((text_width % avilable_width) == 0)?text_width / avilable_width :(text_width / avilable_width) + 1;
	//int lines_needed = ((strlength % max_chars_per_line) == 0) ? strlength / max_chars_per_line : (strlength / max_chars_per_line) + 1;
	char *buff = (char *)malloc(max_chars_per_line+1);

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


	for(int i=0;i<lines_needed;i++){
		int len = snprintf(buff, max_chars_per_line + 1, "%s", string + i * max_chars_per_line);
		XDrawString(display, *win, *gc, x_offset, y_offset + i * (font_info->ascent + font_info->descent), buff, strlen(buff));
	}

/*
	int _strlen = 0;

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
}