#include "gameoflife_sdl.h"

SDLContext *init_sdl(int rows, int cols, int speed) {
  SDLContext *context = malloc(sizeof(SDLContext));
  if (!context)
    return NULL;

  context->simulation_speed = speed;

  if (SDL_Init(SDL_INIT_VIDEO) < 0 || TTF_Init() < 0) {
    printf("Initialization failed: %s\n", SDL_GetError());
    free(context);
    return NULL;
  }

// Chemin de la police différent selon l'OS
#ifdef _WIN32
  context->font =
      TTF_OpenFont("fonts/DejaVuSans.ttf",
                   16); // La police doit être dans le même dossier que l'exe
#else
  context->font =
      TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16);
#endif

  if (!context->font) {
    printf("Font loading failed: %s\n", TTF_GetError());
    TTF_Quit();
    SDL_Quit();
    free(context);
    return NULL;
  }

  context->cell_size = 10;
  context->offset_x = 0;
  context->offset_y = 0;
  context->paused = 0;
  context->zoom = 1.0f;

  int cell_width = WINDOW_WIDTH / cols;
  int cell_height = WINDOW_HEIGHT / rows;
  context->cell_size = (cell_width < cell_height) ? cell_width : cell_height;
  if (context->cell_size > MAX_CELL_SIZE)
    context->cell_size = MAX_CELL_SIZE;
  if (context->cell_size < MIN_CELL_SIZE)
    context->cell_size = MIN_CELL_SIZE;

  context->window = SDL_CreateWindow(
      "Game of Life",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);

  if (!context->window) {
    printf("Window creation failed: %s\n",
           SDL_GetError()); // Message d'erreur pour le debug SDL
    TTF_CloseFont(context->font);
    TTF_Quit();
    SDL_Quit();
    free(context);
    return NULL;
  }

  context->renderer =
      SDL_CreateRenderer(context->window, -1,
                         SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  if (!context->renderer) {
    printf("Renderer creation failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(context->window);
    TTF_CloseFont(context->font);
    TTF_Quit();
    SDL_Quit();
    free(context);
    return NULL;
  }

  context->running = 1;
  return context;
}

void cleanup_sdl(SDLContext *context) {
  if (context->font) {
    TTF_CloseFont(context->font);
  }
  if (context->renderer) {
    SDL_DestroyRenderer(context->renderer);
  }
  if (context->window) {
    SDL_DestroyWindow(context->window);
  }
  TTF_Quit();
  SDL_Quit();
  free(context);
}

void render_board(SDLContext *context, Board *board) {
  if (!context || !board || !context->renderer)
    return;

  SDL_SetRenderDrawColor(context->renderer, 0, 0, 0, 255);
  SDL_RenderClear(context->renderer);

  int effective_cell_size = (int)(context->cell_size * context->zoom);
  int total_width = board->cols * effective_cell_size;
  int total_height = board->rows * effective_cell_size;

  int base_x = (WINDOW_WIDTH - total_width) / 2 + context->offset_x;
  int base_y = (WINDOW_HEIGHT - total_height) / 2 + context->offset_y;

  // Comptage des cellules
  int alive_cells = 0;
  int dead_cells = 0;
  for (int i = 0; i < board->rows; i++) {
    for (int j = 0; j < board->cols; j++) {
      SDL_Rect cell = {base_x + (j * effective_cell_size),
                       base_y + (i * effective_cell_size),
                       effective_cell_size - 1, effective_cell_size - 1};

      if (board->cells[i][j].state == ALIVE) {
        SDL_SetRenderDrawColor(context->renderer, 0, 255, 0, 255);
        alive_cells++;
      } else {
        SDL_SetRenderDrawColor(context->renderer, 32, 32, 32, 255);
        dead_cells++;
      }
      SDL_RenderFillRect(context->renderer, &cell);
    }
  }

  // Rendu des statistiques dans le coin supérieur gauche
  char stats[100];
  snprintf(stats, sizeof(stats), "Gen: %d | Vivantes: %d | Mortes: %d | %s",
           board->generation, alive_cells, dead_cells,
           context->paused ? "PAUSE" : "EN COURS");

  SDL_Color text_color = {255, 255, 255, 255};
  SDL_Surface *text_surface =
      TTF_RenderText_Blended(context->font, stats, text_color);
  SDL_Texture *text_texture =
      SDL_CreateTextureFromSurface(context->renderer, text_surface);

  SDL_Rect text_rect = {10, 10, text_surface->w, text_surface->h};
  SDL_RenderCopy(context->renderer, text_texture, NULL, &text_rect);

  SDL_FreeSurface(text_surface);
  SDL_DestroyTexture(text_texture);

  SDL_RenderPresent(context->renderer);
}

void handle_events(SDLContext *context, Board *board) {
  SDL_Event event;
  const Uint8 *keyboard = SDL_GetKeyboardState(NULL);

  if (keyboard[SDL_SCANCODE_UP])
    context->offset_y += PAN_SPEED;
  if (keyboard[SDL_SCANCODE_DOWN])
    context->offset_y -= PAN_SPEED;
  if (keyboard[SDL_SCANCODE_LEFT])
    context->offset_x += PAN_SPEED;
  if (keyboard[SDL_SCANCODE_RIGHT])
    context->offset_x -= PAN_SPEED;

  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT:
      context->running = 0;
      break;

    case SDL_KEYDOWN:
      switch (event.key.keysym.sym) {
      case SDLK_SPACE:
        context->paused = !context->paused;
        break;
      case SDLK_r: // Reset zoom et position
        context->zoom = 1.0f;
        context->offset_x = 0;
        context->offset_y = 0;
        break;
      case SDLK_d: // Prochaine génération
        if (context->paused) {
          generate_next_cells(board);
        }
        break;
      case SDLK_s: // Sauvegarder
        if (context->paused) {
          save_current_state(board);
        }
        break;
      case SDLK_q: // Précédente génération
        if (context->paused && board->prev) {
          Board *prev = board->prev;

          // Copie de l'état précédent
          for (int i = 0; i < board->rows; i++) {
            memcpy(board->cells[i], prev->cells[i], board->cols * sizeof(Cell));
          }
          board->generation = prev->generation;

          // Update du pointeur de génération précédente
          board->prev = prev->prev;

          // Libération de la mémoire
          for (int i = 0; i < prev->rows; i++) {
            free(prev->cells[i]);
          }
          free(prev->cells);
          free(prev);
        }
        break;
      }
      break;

    case SDL_MOUSEWHEEL: // Zoom
      if (event.wheel.y > 0) {
        context->zoom *= 1.1f;
        if (context->zoom > 5.0f)
          context->zoom = 5.0f;
      } else if (event.wheel.y < 0) {
        context->zoom *= 0.9f;
        if (context->zoom < 0.2f)
          context->zoom = 0.2f;
      }
      break;
    }
  }
}

void save_current_state(Board *board) {
  // Taille des buffers
  char filename[512];
  char full_filename[512];

  // Récupérer la date actuelle
  time_t t = time(NULL);
  struct tm *tm_info = localtime(&t);

  // Génération le nom de base du fichier avec la date
  strftime(filename, sizeof(filename), "game_of_life_%Y%m%d_%H%M%S_gen",
           tm_info);

  // Nommage du fichier avec le numéro de génération actuel
  int result = snprintf(full_filename, sizeof(full_filename),
                        "exports/%s%d.txt", filename, board->generation);

  // Vérifiez si le nom a été tronqué
  if (result >= sizeof(full_filename)) {
    fprintf(stderr, "Erreur : le nom de fichier a été tronqué.\n");
    exit(EXIT_FAILURE);
  }

  // Sauvegarde de l'état actuel en fichier txt
  export_board(board, full_filename);

  // Message de confirmation
  printf("\nÉtat sauvegardé dans : %s\n", full_filename);
}
