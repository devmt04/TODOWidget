run :
	gcc -o main main.c utils.c -lX11 -lXext && ./main

build :
	gcc -o main main.c utils.c -lX11 -lXext

debug :
	gcc -g -o main main.c utils.c -lX11 -lXext && gdb ./main