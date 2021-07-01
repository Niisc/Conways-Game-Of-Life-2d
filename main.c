#include "include/raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <pthread.h>

typedef struct _Square {
	uint32_t PosX;
	uint32_t PosY;
} Square;

typedef struct _ListArray
{
    size_t size;
    size_t actual_size;
    Square *content;
} ListArray;

#define MAX_THREADS 13
#define THREADSTACK  131072
pthread_attr_t  attrs;
pthread_t pid[MAX_THREADS] = {0};
pthread_mutex_t mutex;
uint32_t thread_counts[MAX_THREADS] = {0};

#define TSIZEE 100 //This is grid size, change to any number you'd like
uint8_t TABLE[TSIZEE][TSIZEE];
uint8_t TABLE2[TSIZEE][TSIZEE];
ListArray LIVE_SQUARES;
ListArray LIVE_SQUARES2;

void create_array(ListArray *d);
void element_append(ListArray *d, Square v);
void destroy_array(ListArray *d);
size_t array_size(ListArray *d);
uint8_t delete(ListArray *v, int index);
uint8_t surrounding_cells(Square* square);
uint8_t surrounding_cellss(long PosXX, long PosYY);
void do_stuff();
void *CellCheck(void * pv);
uint8_t cell_exists(long PosX, long PosY);
void print_table();
void create_random_startup();
uint16_t num = 0;
double simulation_speed = 0.5;
int main(void)
{
	srand(time(NULL)); 
    create_array(&LIVE_SQUARES);
    create_array(&LIVE_SQUARES2);
	create_random_startup();

    const int screenWidth = 1920;
    const int screenHeight = 1080;

    InitWindow(screenWidth, screenHeight, "Conways game of life");
    ToggleFullscreen();
    Camera2D camera = { 0 };
    camera.zoom = 1.0f;
    camera.offset = (Vector2){ (screenWidth - TSIZEE )/2.0f, (screenHeight- TSIZEE )/2.0f };
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {

        if (IsKeyDown(KEY_A)) camera.target.x -= 2;
        else if (IsKeyDown(KEY_D)) camera.target.x += 2;
        else if (IsKeyDown(KEY_W)) camera.target.y -=2;
        else if (IsKeyDown(KEY_S)) camera.target.y +=2;

        camera.zoom += ((float)GetMouseWheelMove()*0.1f);
        
        if (IsKeyPressed(KEY_Q)){
            num = 0;   
            simulation_speed *= 2; 
            floor(simulation_speed);
        }
        else if (IsKeyPressed(KEY_E)){
            num = 0;
            simulation_speed /= 2;
            floor(simulation_speed);
        } 

        if (IsKeyPressed(KEY_R))
        {
            camera.zoom = 5.0f;
            camera.rotation = 0.0f;
        }
        if (simulation_speed >= 1)
        {
            for (size_t i = 0; i < simulation_speed; i++)
            {
                do_stuff();
            }
        }else{
            if (num * simulation_speed != 1)
            {
                num++;
            }else{
                do_stuff();
                num = 0;
            }
            
        }
        
        BeginDrawing();

            ClearBackground(RAYWHITE);
            BeginMode2D(camera);
            for (size_t i = 0; i < TSIZEE; i++)
            {
                for (size_t k = 0; k < TSIZEE; k++)
                {
                    TABLE[i][k] ? DrawRectangle(i,k,1,1,WHITE) : DrawRectangle(i,k,1,1,BLACK);
                }
            }

            EndMode2D();
            DrawFPS(0,0);
            DrawText("Free 2d camera controls:", 20, 20, 10, BLACK);
            DrawText("- WASD to move around", 40, 40, 10, DARKGRAY);
            DrawText("- Mouse Wheel to Zoom in-out", 40, 60, 10, DARKGRAY);
            DrawText("- R to reset Zoom and Rotation", 40, 1800, 10, DARKGRAY);
            DrawText("- Q / E to control the simulations speed", 40, 100, 10, DARKGRAY);
            DrawText(TextFormat("Current simulation speed %lf",simulation_speed), 40, 120, 10, DARKGRAY);
        EndDrawing();
    }

    CloseWindow();        // Close window and OpenGL context

    return 0;
}

void create_array(ListArray *d)
{
	Square* temp = malloc(sizeof(struct Square*));
	if (!temp)
    {
        fprintf(stderr, "Failed to allocate array");            
		exit(EXIT_FAILURE);
    }
	d->actual_size = d->size = 0;
	d->content= temp;
}

void element_append(ListArray *d, Square v)
{
    if (d->size+1 > d->actual_size)
    {
        size_t new_size;
        if (!d->actual_size) 
        { 
            new_size = 1;
        }
        else
        {
            new_size = d->actual_size * 2;
        }
        Square *temp = realloc(d->content, sizeof(Square) * new_size);
        if (!temp)
        {
            fprintf(stderr, "Failed to extend array (new_size=%zu)\n", new_size);
            exit(EXIT_FAILURE);
        }
        d->actual_size = new_size;
        d->content = temp;
    }
    d->content[d->size] = v;
    d->size++;
}

void destroy_array(ListArray *d)
{
    free(d->content);
    d->content = NULL;
    d->size = d->actual_size = 0;
}

size_t array_size(ListArray *d)
{
    return d->size;
}

uint8_t delete(ListArray *v, int index)
{
    if (index < 0 || index >= v->size)
        return -1;

    memmove(v->content + index, v->content + index + 1, (v->size - index - 1) * sizeof(Square));
    v->size--;
    if (v->actual_size / 2 > v->size)
    {
        size_t new_size;
        new_size = v->actual_size / 2;
        Square *temp = realloc(v->content, sizeof(Square) * new_size);
        if (!temp)
        {
            fprintf(stderr, "Failed to extend array (new_size=%zu)\n", new_size);
            exit(EXIT_FAILURE);
        }
        v->actual_size = new_size;
        v->content = temp;
    }
    return 0;
}

void create_random_startup() {
	for (long i = 0; i < TSIZEE; i++)
	{
		for (long k = 0; k < TSIZEE; k++)
		{
			TABLE[i][k] = (rand() % 2) && (rand() % 2) && (rand() % 2); 
			if (TABLE[i][k])
			{
				Square square;
				square.PosX = i;
				square.PosY = k;
				element_append(&LIVE_SQUARES, square);
			}
		}
	}
    
    if (pthread_attr_init(&attrs) != 0)
    {
        printf("\n attrs set stack size init failed\n");
        exit(1);
    }
	if (pthread_attr_setstacksize(&attrs, THREADSTACK) != 0)
    {
        printf("\n attrs set stack size init failed\n");
        exit(1);
    }
    if (pthread_mutex_init(&mutex, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        exit(1);
    }
	for (size_t i = 0; i < TSIZEE; i++)
		for (size_t k = 0; k < TSIZEE; k++)
			TABLE2[i][k] = TABLE[i][k];

}
uint8_t surrounding_cells(Square* square) {
	uint8_t numOfCells = 0;
	long posx, posy;
	for (long i = -1; i < 2; i++)
	{
		posx = square->PosX + i;
		for (long k = -1; k < 2; k++)
		{
			posy = square->PosY + k;
			if (cell_exists(posx, posy) && TABLE[posx][posy] && !((posx == square->PosX)&&(posy == square->PosY)))
			{
				numOfCells++;
			}
		}
	}
	return numOfCells;
}

uint8_t surrounding_cellss(long PosXX, long PosYY) {
	uint8_t numOfCells = 0;
	long posx, posy;
	for (long i = -1; i < 2; i++)
	{
		posx = PosXX + i;
		for (long k = -1; k < 2; k++)
		{
			posy = PosYY + k;
			if (cell_exists(posx, posy) && TABLE[posx][posy] && !((posx == PosXX)&&(posy == PosYY)))
			{
				numOfCells++;
			}
		}
	}
	return numOfCells;
}

void do_stuff() {
    uint16_t  thread_pos = 0;

	int  err;
    uint32_t amount_of_cycles = LIVE_SQUARES.size * (LIVE_SQUARES.size < MAX_THREADS) + MAX_THREADS * (MAX_THREADS < LIVE_SQUARES.size) + MAX_THREADS * (MAX_THREADS == LIVE_SQUARES.size); 
    for (thread_pos = 0; thread_pos < amount_of_cycles; thread_pos++) {

        thread_counts[thread_pos] = thread_pos;
		err = pthread_create(&(pid[thread_pos]), &attrs, &CellCheck, &thread_counts[thread_pos]);
		if (err != 0){
            printf("Problem occured in creading threads");
			break;
		} 
    }
	
	for (size_t i = 0; i < (thread_pos); i++) 
	{
		pthread_join(pid[i], NULL);
	}

	for (size_t i = 0; i < TSIZEE; i++)
		for (size_t k = 0; k < TSIZEE; k++)
			TABLE[i][k] = TABLE2[i][k];

    destroy_array(&LIVE_SQUARES);
    create_array(&LIVE_SQUARES);
    LIVE_SQUARES = LIVE_SQUARES2;
    create_array(&LIVE_SQUARES2);
}

void *CellCheck(void * pv){
    size_t size = LIVE_SQUARES.size;
    Square cSquare;
	uint8_t stuff;
    uint8_t o = 0;
    uint8_t x = 0;
    uint32_t numOfStored = ((*((uint32_t*) pv)) + 1)*77;
    Square *aSquare = malloc(sizeof(Square)*numOfStored*10);
	long posx, posy;
    for (uint32_t currentpos = *((uint32_t*) pv); currentpos < size; currentpos += MAX_THREADS)
    {
        cSquare = LIVE_SQUARES.content[currentpos];
		stuff = surrounding_cells(&cSquare);
		if ((stuff != 2) && (stuff != 3))
		{
			TABLE2[cSquare.PosX][cSquare.PosY] = 0;
		}
		else
		{
            aSquare[o] = cSquare;
            o++;
		}
        
		for (long i = -1; i < 2; i++)
		{
			posx = cSquare.PosX + i;
			for (long k = -1; k < 2; k++)
			{
				posy = cSquare.PosY + k;
				if (cell_exists(posx, posy) && !TABLE2[posx][posy] && surrounding_cellss(posx, posy) == 3)
				{
                    Square gSquare;
					gSquare.PosX = posx;
					gSquare.PosY = posy;
					TABLE2[posx][posy] = 1;
                    aSquare[o] = gSquare;
                    o++;
				}
			}
		}
        x++;
        if (x == numOfStored)
        {
            pthread_mutex_lock(&mutex);
            for (size_t i = 0; i < o; i++)
            {
				element_append(&LIVE_SQUARES2, aSquare[i]);
            }
            pthread_mutex_unlock(&mutex);
            free(aSquare);
            aSquare = malloc(sizeof(Square)*numOfStored*10);
            x=0;
            o=0;
        }
    }
    if (x != 0)
    {
        pthread_mutex_lock(&mutex);
        for (size_t i = 0; i < o; i++)
        {
		    element_append(&LIVE_SQUARES2, aSquare[i]);
        }
        pthread_mutex_unlock(&mutex);
    }
    free(aSquare);
}

uint8_t cell_exists(long PosX, long PosY) {
	return !(PosX < 0 || PosY < 0 || PosX >= TSIZEE || PosY >= TSIZEE);
}

