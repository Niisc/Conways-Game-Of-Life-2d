gcc main.c -g -std=c99 -c -I /include -o main.o
gcc main.o -s -Wall -std=c99 -I/include -L/lib -l:libraylib.a -lGL -lm -lpthread -ldl -lrt -lX11 -O3
