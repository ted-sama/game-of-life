#include <stdio.h>
#include <stdlib.h>
#include "gameoflife_sdl.h"

int get_valid_input(int min, int max, const char *prompt) {
    int value;
    int valid = 0;
    
    do {
        printf("%s (%d-%d): ", prompt, min, max);
        if (scanf("%d", &value) == 1) {
            if (value >= min && value <= max) {
                valid = 1;
            } else {
                printf("La valeur doit être entre %d et %d.\n", min, max);
            }
        } else {
            printf("Entrée invalide. Veuillez entrer un nombre.\n");
            while (getchar() != '\n'); // Nettoie le buffer
        }
    } while (!valid);
    
    return value;
}

char *get_filename() {
    char *filename = malloc(512);
    if (!filename) {
        fprintf(stderr, "Erreur : Échec de l'allocation mémoire pour filename.\n");
        exit(EXIT_FAILURE);
    }

    printf("Entrez le nom du planeur à charger (ex. pulsar): ");
    if (scanf("%511s", filename) != 1) { // Limite la lecture à 511 caractères
        fprintf(stderr, "Erreur : Échec de la lecture du nom du planeur.\n");
        free(filename);
        exit(EXIT_FAILURE);
    }

    // Alloue de la mémoire pour le chemin complet
    char *glider = malloc(512);
    if (!glider) {
        fprintf(stderr, "Erreur : Échec de l'allocation mémoire pour glider.\n");
        free(filename);
        exit(EXIT_FAILURE);
    }

    // Nomme le chemin complet
    snprintf(glider, 512, "gliders/%s.txt", filename);

    free(filename); // Libère la mémoire du nom du planeur
    return glider;
}

int get_simulation_speed() {
    int speed;
    printf("Entrez la vitesse de simulation (en ms entre chaque génération, 100-2000): ");
    scanf("%d", &speed);
    if (speed < 100) speed = 100;
    if (speed > 2000) speed = 2000;
    return speed;
}

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