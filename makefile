routing: routing.c tools.c packbuf.c routing.h
	gcc routing.c tools.c packbuf.c -o routing

run:
	sudo ./routing
