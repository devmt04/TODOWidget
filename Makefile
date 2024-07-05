run :
	gcc -o main main.c utils.c database.c -lX11 -lXext -lXcomposite -lXrender -lsqlite3 -lXft -I/usr/include/freetype2  && ./main

build :
	gcc -o main main.c utils.c database.c -lX11 -lXext -lXcomposite -lXrender -lsqlite3 -lXft -I/usr/include/freetype2

debug :
	gcc -g -o main main.c utils.c database.c -lX11 -lXext -lXcomposite -lXrender -lsqlite3 -lXft -I/usr/include/freetype2 && gdb ./main