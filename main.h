#define MWM_HINTS_DECORATIONS 2

typedef struct {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long input_mode;
    unsigned long status;
} MotifWmHints;


typedef struct TodoItem{
    Window win;
    int id;
    char *data;
    int datalen;
    char *date;
    struct TodoItem *prev;
    struct TodoItem *next;
} TodoItem;
