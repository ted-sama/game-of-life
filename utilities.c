#include "utilities.h"

// Fonction pour obtenir une entrée valide de l'utilisateur quand il
// initiatilise le jeu
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
      while (getchar() != '\n')
        ; // Nettoie le buffer
    }
  } while (!valid);

  return value;
}

// Fonction pour obtenir le nom du fichier à charger depuis le dossier 'gliders
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

// Fonction pour obtenir la vitesse de simulation de l'utilisateur
int get_simulation_speed() {
  int speed;
  printf("Entrez la vitesse de simulation (en ms entre chaque génération, "
         "100-2000): ");
  scanf("%d", &speed);
  if (speed < 100)
    speed = 100;
  if (speed > 2000)
    speed = 2000;
  return speed;
}