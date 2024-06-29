run :
	gcc -o main main.c utils.c database.c -lX11 -lXext -lsqlite3 && ./main

build :
	gcc -o main main.c utils.c database.c -lX11 -lXext -lsqlite3

debug :
	gcc -g -o main main.c utils.c database.c -lX11 -lXext -lsqlite3 && gdb ./main