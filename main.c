#include "gameoflife_sdl.h"
#include "utilities.h"

int main(int argc, char *argv[]) {
    // Demande des dimensions à l'utilisateur
    printf("Bienvenue dans le Jeu de la Vie!\n");
    printf("Veuillez choisir les dimensions de la grille.\n");
    
    int rows = get_valid_input(1, MAX_ROWS, "Nombre de lignes");
    int cols = get_valid_input(1, MAX_COLS, "Nombre de colonnes");
    char *glider = get_filename();
    int speed = get_simulation_speed();

    // Initialisation de SDL
    SDLContext *sdl = init_sdl(rows, cols, speed);
    if (!sdl) {
        fprintf(stderr, "Failed to initialize SDL\n");
        return 1;
    }

    // Création du plateau avec les dimensions choisies
    Board *board = create_board(rows, cols);
    if (!board) {
        fprintf(stderr, "Failed to create board\n");
        cleanup_sdl(sdl);
        return 1;
    }

    // Vérifier si le fichier existe et initialiser le board
    FILE *test = fopen(glider, "r");
    if (test != NULL) {
        fclose(test);
        import_board(board, glider);
    } else {
        // Si le fichier n'existe pas, on crée un planeur simple si les dimensions le permettent
        if (rows > 3 && cols > 3) {
            board->cells[1][2].state = ALIVE;
            board->cells[2][3].state = ALIVE;
            board->cells[3][1].state = ALIVE;
            board->cells[3][2].state = ALIVE;
            board->cells[3][3].state = ALIVE;
        }
    }

    // Affichage des commandes dans le terminal pour l'utilisateur
    printf("\nCommandes:\n");
    printf("- ESPACE : Pause/Reprise\n");
    printf("- Flèches : Déplacement de la vue\n");
    printf("- Molette : Zoom\n");
    printf("- R : Reset vue\n");
    printf("- Q (en pause) : Génération précédente\n");
    printf("- D (en pause) : Génération suivante\n");
    printf("- S (en pause) : Sauvegarder l'état actuel\n");
    printf("\nAppuyez sur Entrée pour commencer...");
    while (getchar() != '\n');
    getchar();

    Uint32 lastTime = 0;

    while (sdl->running) {
        Uint32 currentTime = SDL_GetTicks();
        
        handle_events(sdl, board);
        render_board(sdl, board);
        
        if (!sdl->paused && currentTime - lastTime >= sdl->simulation_speed) {
            generate_next_cells(board);
            lastTime = currentTime;
        }
        
        SDL_Delay(1);
    }

    destroy_board(board);
    cleanup_sdl(sdl);
    return 0;
}