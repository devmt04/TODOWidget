run :
	gcc -o main main.c utils.c -lX11 && ./main

build :
	gcc -o main main.c utils.c -lX11

debug :
	gcc -g -o main main.c utils.c -lX11 && gdb ./main