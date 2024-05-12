#ifndef RENDER
#define RENDER

#include "audiowave.h"


void select();
void clear();

void mouse_input(int, int);

void render(struct window_buffer);

#endif

