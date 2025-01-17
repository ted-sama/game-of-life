#ifndef GAMEOFLIFE_SDL_H
#define GAMEOFLIFE_SDL_H

#include "gameoflife.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_ROWS 90
#define MAX_COLS 300
#define MIN_CELL_SIZE 3
#define MAX_CELL_SIZE 50
#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#define PAN_SPEED 30

// Structure pour le message de sauvegarde
typedef struct {
  char text[256];
  int display_time;
  Uint32 start_time;
} SaveMessage;

// Structure de contexte SDL (fenêtre, rendu, police, etc.)
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
  SaveMessage save_message;
} SDLContext;

SDLContext *init_sdl(int rows, int cols, int speed);
void cleanup_sdl(SDLContext *context);
SDL_Texture *render_text(SDLContext *context, const char *text, SDL_Color color,
                         int *w, int *h);
void render_help(SDLContext *context);
void render_save_message(SDLContext *context);
void render_board(SDLContext *context, Board *board);
void handle_events(SDLContext *context, Board *board);
void save_current_state(Board *board);

#endif