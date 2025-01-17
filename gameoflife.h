#ifndef GAMEOFLIFE_H
#define GAMEOFLIFE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define ALIVE_CHAR '\u25A0'
#define DEAD_CHAR '\u25A1'

typedef enum { ALIVE, DEAD } State;

typedef struct {
  State state;
} Cell;

typedef struct Board {
  int rows;
  int cols;
  Cell **cells;
  int generation;
  struct Board *prev; // Pointeur vers la génération précédente pour le undo
  struct Board *next; // Pointeur vers la génération suivante pour le redo
} Board;

Board *create_board(int rows, int cols);
void resize_board(Board *board, int rows, int cols);
void destroy_board(Board *board);
void import_board(Board *board, char *filename);
void export_board(Board *board, char *filename);
int print_board(Board *board);
void generate_next_cells(Board *board);

#endif