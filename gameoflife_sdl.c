#include "gameoflife_sdl.h"

SDLContext *init_sdl(int rows, int cols, int speed) {
  SDLContext *context = malloc(sizeof(SDLContext));
  if (!context)
    return NULL;

  context->simulation_speed = speed;
  context->save_message.display_time = 0;
  context->save_message.text[0] = '\0';

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

  context->window = SDL_CreateWindow("Game of Life", SDL_WINDOWPOS_CENTERED,
                                     SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH,
                                     WINDOW_HEIGHT, SDL_WINDOW_SHOWN);

  if (!context->window) {
    printf("Window creation failed: %s\n",
           SDL_GetError()); // Message d'erreur pour le debug SDL
    TTF_CloseFont(context->font);
    TTF_Quit();
    SDL_Quit();
    free(context);
    return NULL;
  }

  SDL_SetWindowData(context->window, "sdl_context", context);

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

// Fonction pour rendre le texte - extrait pour réutilisation
SDL_Texture *render_text(SDLContext *context, const char *text, SDL_Color color,
                         int *w, int *h) {
  SDL_Surface *surface = TTF_RenderUTF8_Blended(context->font, text, color);
  if (!surface)
    return NULL;

  *w = surface->w;
  *h = surface->h;
  SDL_Texture *texture =
      SDL_CreateTextureFromSurface(context->renderer, surface);
  SDL_FreeSurface(surface);

  return texture;
}

void render_help(SDLContext *context) {
  SDL_Color text_color = {200, 200, 200, 255};
  const char *commands[] = {"Commandes:",
                            "ESPACE : Pause/Reprise",
                            "Flèches : Déplacement",
                            "Molette : Zoom",
                            "R : Reset vue",
                            "Q : Génération précédente",
                            "D : Génération suivante",
                            "S : Sauvegarder l'état"};

  int x = WINDOW_WIDTH - 250; // Position X fixe pour la liste
  int y = 10;                 // Commence en haut
  int line_height = 20;       // Espacement entre les lignes

  for (int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
    int w, h;
    SDL_Texture *text = render_text(context, commands[i], text_color, &w, &h);
    if (text) {
      SDL_Rect dest = {x, y + (i * line_height), w, h};
      SDL_RenderCopy(context->renderer, text, NULL, &dest);
      SDL_DestroyTexture(text);
    }
  }
}

void set_save_message(SDLContext *context, const char *message) {
  if (context) {
    strncpy(context->save_message.text, message,
            sizeof(context->save_message.text) - 1);
    context->save_message.text[sizeof(context->save_message.text) - 1] = '\0';
    context->save_message.display_time = 3000;
    context->save_message.start_time = SDL_GetTicks();
  }
}

void render_save_message(SDLContext *context) {
  if (context && context->save_message.display_time > 0) {
    Uint32 current_time = SDL_GetTicks();
    Uint32 elapsed = current_time - context->save_message.start_time;

    if (elapsed < context->save_message.display_time) {
      // Calcul de l'alpha pour le fade out
      float fade = 1.0f - ((float)elapsed / context->save_message.display_time);
      SDL_Color msg_color = {0, 255, 0, 255}; // Couleur verte

      int w, h;
      // AVerifier si le contexte est valide pour afficher le message
      if (!context->renderer) {
        return;
      }

      SDL_Texture *text =
          render_text(context, context->save_message.text, msg_color, &w, &h);
      if (text) {
        SDL_SetTextureAlphaMod(text, (Uint8)(fade * 255));

        SDL_Rect dest = {(WINDOW_WIDTH - w) / 2, WINDOW_HEIGHT - 100, w, h};

        // Settings de blend pour le rendu du texte
        SDL_BlendMode previousBlendMode;
        SDL_GetRenderDrawBlendMode(context->renderer, &previousBlendMode);

        SDL_SetRenderDrawBlendMode(context->renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderCopy(context->renderer, text, NULL, &dest);

        SDL_SetRenderDrawBlendMode(context->renderer, previousBlendMode);

        SDL_DestroyTexture(text);
      }
    } else {
      context->save_message.display_time = 0;
      context->save_message.text[0] = '\0';
    }
  }
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
  int w, h;
  SDL_Texture *stats_texture = render_text(context, stats, text_color, &w, &h);
  if (stats_texture) {
    SDL_Rect dest = {10, 10, w, h};
    SDL_RenderCopy(context->renderer, stats_texture, NULL, &dest);
    SDL_DestroyTexture(stats_texture);
  }

  // Affichage des commandes
  render_help(context);

  // Affichage du message de sauvegarde
  render_save_message(context);

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
          if (board->next) {
            // Si une génération suivante existe, on la restaure
            Board *next = board->next;

            // Copie de l'état suivant
            for (int i = 0; i < board->rows; i++) {
              memcpy(board->cells[i], next->cells[i],
                     board->cols * sizeof(Cell));
            }
            board->generation = next->generation;

            // Update des pointeurs
            board->next = next->next;
            if (board->next) {
              board->next->prev = board;
            }

            // Libération de la mémoire
            for (int i = 0; i < next->rows; i++) {
              free(next->cells[i]);
            }
            free(next->cells);
            free(next);
          } else {
            // Sinon on génère une nouvelle génération
            generate_next_cells(board);
          }
        }
        break;
      case SDLK_s: // Sauvegarder
        if (context->paused) {
          save_current_state(context, board);
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

          // Update des pointeurs
          board->prev = prev->prev;
          if (board->prev) {
            board->prev->next = board;
          }

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

void save_current_state(SDLContext *context, Board *board) {
  char filename[256];
  char full_filename[512];

  time_t t = time(NULL);
  struct tm *tm_info = localtime(&t);

  strftime(filename, sizeof(filename), "game_of_life_%Y%m%d_%H%M%S_gen",
           tm_info);

  if (snprintf(full_filename, sizeof(full_filename), "exports/%s_%d.txt",
               filename, board->generation) >= sizeof(full_filename)) {
    fprintf(stderr, "Erreur : nom de fichier trop long\n");
    return;
  }

  // Créer le dossier exports s'il n'existe pas (Windows/Linux)
  create_directory("exports");

  export_board(board, full_filename);

  // Extraire le nom du fichier sans le chemin pour raccourcir le message
  const char *base_name = strrchr(full_filename, '/');
  base_name = base_name ? base_name + 1 : full_filename;

  // Définir directement le message puisque nous avons le contexte
  char message[256];
  snprintf(message, sizeof(message), "Sauvegardé : %s", base_name);

  // On s'assure que le contexte existe avant de définir le message
  if (context) {
    strncpy(context->save_message.text, message,
            sizeof(context->save_message.text) - 1);
    context->save_message.text[sizeof(context->save_message.text) - 1] = '\0';
    context->save_message.display_time = 3000;
    context->save_message.start_time = SDL_GetTicks();
  }

  printf("\nÉtat sauvegardé dans : %s\n", full_filename);
}
