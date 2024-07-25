#ifndef SNAKE_H
#define SNAKE_H

#include "cmsis_os.h"

// Funkcje gry Snake
void Snake_Init(void);
void Snake_Run(void *argument);
void Snake_Reset(void);

#endif // SNAKE_H
