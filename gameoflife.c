#include "gameoflife.h"

Board *create_board(int rows, int cols) {
  Board *board = malloc(sizeof(Board));
  board->rows = rows;
  board->cols = cols;
  board->cells = malloc(rows * sizeof(Cell *));
  board->generation = 0;
  board->prev = NULL;
  board->next = NULL;
  for (int i = 0; i < rows; i++) {
    board->cells[i] = malloc(cols * sizeof(Cell));
  }
  // On initialise toutes les cellules à mortes pour commencer
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      board->cells[i][j].state = DEAD;
    }
  }
  return board;
}

void resize_board(Board *board, int rows, int cols) {
  board->cells = realloc(board->cells, rows * sizeof(Cell *));
  for (int i = 0; i < rows; i++) {
    board->cells[i] = realloc(board->cells[i], cols * sizeof(Cell));
  }
  if (rows > board->rows) {
    for (int i = board->rows; i < rows; i++) {
      for (int j = 0; j < cols; j++) {
        board->cells[i][j].state = DEAD;
      }
    }
  }
  if (cols > board->cols) {
    for (int i = 0; i < rows; i++) {
      for (int j = board->cols; j < cols; j++) {
        board->cells[i][j].state = DEAD;
      }
    }
  }
  board->rows = rows;
  board->cols = cols;
}

void destroy_board(Board *board) {
  // Pour chaque génération précédente, libérer la mémoire
  while (board->prev) {
    Board *prev = board->prev;
    for (int i = 0; i < prev->rows; i++) {
      free(prev->cells[i]);
    }
    free(prev->cells);
    free(prev);
    board->prev = NULL;
  }

  // Pour chaque génération suivante, libérer la mémoire
  while (board->next) {
    Board *next = board->next;
    for (int i = 0; i < next->rows; i++) {
      free(next->cells[i]);
    }
    free(next->cells);
    free(next);
    board->next = NULL;
  }

  // Free current board
  for (int i = 0; i < board->rows; i++) {
    free(board->cells[i]);
  }
  free(board->cells);
  free(board);
}

void import_board(Board *board, char *filename) {
  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    printf("Error: File not found\n");
    return;
  }
  for (int i = 0; i < board->rows; i++) {
    for (int j = 0; j < board->cols; j++) {
      char c;
      fscanf(file, "%c", &c);
      if (c == 'O') {
        board->cells[i][j].state = ALIVE;
      } else if (c == '.') {
        board->cells[i][j].state = DEAD;
      }
    }
    fscanf(file, "\n");
  }
  fclose(file);
}

void export_board(Board *board, char *filename) {
  FILE *file = fopen(filename, "w");
  for (int i = 0; i < board->rows; i++) {
    for (int j = 0; j < board->cols; j++) {
      if (board->cells[i][j].state == ALIVE) {
        fprintf(file, "O");
      } else if (board->cells[i][j].state == DEAD) {
        fprintf(file, ".");
      }
    }
    fprintf(file, "\n");
  }
  fclose(file);
}

// Affiche le plateau de jeu pour la version terminal
int print_board(Board *board) {
  int alive_cells = 0;
  for (int i = 0; i < board->rows; i++) {
    for (int j = 0; j < board->cols; j++) {
      if (board->cells[i][j].state == ALIVE) {
        alive_cells++;
        printf("\033[32m■\033[0m");
      } else if (board->cells[i][j].state == DEAD) {
        // Couleur ANSI 256 pour gris foncé : \033[38;5;240m
        // Réinitialisation de la couleur : \033[0m
        printf("\033[38;5;240m■\033[0m");
      }
    }
    printf("\n");
  }
  board->generation++;

  printf("\n");
  printf("\033[33mGeneration: %d\n\033[0m", board->generation);

  return alive_cells;
}

void generate_next_cells(Board *board) {
  // Si on génère depuis un état qui a déjà un "next", on doit d'abord effacer
  // cet historique
  if (board->next) {
    Board *current_next = board->next;
    while (current_next) {
      Board *to_delete = current_next;
      current_next = current_next->next;
      for (int i = 0; i < to_delete->rows; i++) {
        free(to_delete->cells[i]);
      }
      free(to_delete->cells);
      free(to_delete);
    }
    board->next = NULL;
  }

  // Sauvegarder l'état actuel
  Board *prev = malloc(sizeof(Board));
  prev->rows = board->rows;
  prev->cols = board->cols;
  prev->generation = board->generation;
  prev->cells = malloc(board->rows * sizeof(Cell *));

  for (int i = 0; i < board->rows; i++) {
    prev->cells[i] = malloc(board->cols * sizeof(Cell));
    memcpy(prev->cells[i], board->cells[i], board->cols * sizeof(Cell));
  }

  prev->prev = board->prev;
  if (board->prev) {
    board->prev->next = prev; // L'ancien prev pointe vers le nouveau prev
  }
  board->prev = prev;

  // Allouer la nouvelle grille
  Cell **next_cells = malloc(board->rows * sizeof(Cell *));
  if (next_cells == NULL)
    return;

  for (int i = 0; i < board->rows; i++) {
    next_cells[i] = malloc(board->cols * sizeof(Cell));
    if (next_cells[i] == NULL) {
      // Nettoyer les allocations précédentes en cas d'échec
      for (int j = 0; j < i; j++) {
        free(next_cells[j]);
      }
      free(next_cells);
      return;
    }
  }

  // Calculer la prochaine génération
  for (int i = 0; i < board->rows; i++) {
    for (int j = 0; j < board->cols; j++) {
      int alive_neighbors = 0;

      for (int k = i - 1; k <= i + 1; k++) {
        for (int l = j - 1; l <= j + 1; l++) {
          if (k >= 0 && k < board->rows && l >= 0 && l < board->cols) {
            if (k == i && l == j)
              continue;
            if (board->cells[k][l].state == ALIVE) {
              alive_neighbors++;
            }
          }
        }
      }

      // Appliquer les règles
      if (board->cells[i][j].state == ALIVE) {
        next_cells[i][j].state =
            (alive_neighbors == 2 || alive_neighbors == 3) ? ALIVE : DEAD;
      } else {
        next_cells[i][j].state = (alive_neighbors == 3) ? ALIVE : DEAD;
      }
    }
  }

  // Mettre à jour le board
  for (int i = 0; i < board->rows; i++) {
    memcpy(board->cells[i], next_cells[i], board->cols * sizeof(Cell));
    free(next_cells[i]);
  }
  free(next_cells);

  board->generation++;
}