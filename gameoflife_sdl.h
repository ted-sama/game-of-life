#ifndef GAMEOFLIFE_SDL_H
#define GAMEOFLIFE_SDL_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "gameoflife.h"

#define MAX_ROWS 90
#define MAX_COLS 300
#define MIN_CELL_SIZE 3
#define MAX_CELL_SIZE 50
#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#define PAN_SPEED 30

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *font;
    int running;
    int cell_size;
    int offset_x;
    int offset_y;
    int paused;
    float zoom;
    int simulation_speed;
} SDLContext;

SDLContext* init_sdl(int rows, int cols, int speed);
void cleanup_sdl(SDLContext *context);
void render_board(SDLContext *context, Board *board);
void handle_events(SDLContext *context, Board *board);
void save_current_state(Board *board);

#endif