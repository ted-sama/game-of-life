#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdio.h>
#include <stdlib.h>

int get_valid_input(int min, int max, const char *prompt);
char *get_filename();
int get_simulation_speed();

#endif