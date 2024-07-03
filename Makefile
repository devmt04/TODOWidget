run :
	gcc -o main main.c utils.c database.c -lX11 -lXext -lsqlite3 -lXft -I/usr/include/freetype2 -lfreetype -lfontconfig && ./main

build :
	gcc -o main main.c utils.c database.c -lX11 -lXext -lsqlite3 -lXft -I/usr/include/freetype2 -lfreetype -lfontconfig

debug :
	gcc -g -o main main.c utils.c database.c -lX11 -lXext -lsqlite3 -lXft -I/usr/include/freetype2 -lfreetype -lfontconfig && gdb ./main