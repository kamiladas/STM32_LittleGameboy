#include "snake.h"
#include "main.h" // For access to joystick functions
#include "ssd1306.h" // For access to OLED functions
#include "ssd1306_fonts.h" // For access to fonts
#include <stdlib.h> // For random number generation

// Zmienne globalne dla stanu gry Snake
extern volatile uint8_t longPressDetected;
extern uint32_t yAxisValue;
extern uint32_t xAxisValue;
extern void handleLongPress(void);
extern void displayMessage(const char* message);
extern void displayMenu(void);
extern volatile uint8_t gameRunning; // Flaga wskazująca, czy gra jest uruchomiona
static osThreadId_t snakeTaskHandle;
extern osMutexId_t oledMutexHandle; // Dodanie deklaracji extern dla mutexa OLED

// Definicje stałych
#define JOY_Y_THRESHOLD 300
#define JOY_X_THRESHOLD 300

#define SNAKE_SPEED 4
#define PIXEL_SIZE 4

#define MAX_SNAKE_LENGTH 50
#define SCOREBOARD_HEIGHT 12

static int snakeX[MAX_SNAKE_LENGTH];
static int snakeY[MAX_SNAKE_LENGTH];
static int snakeLength;
static int snakeDirectionX;
static int snakeDirectionY;
static int pointX;
static int pointY;
static int score;

void drawBorders(void) {
    ssd1306_Fill(Black);
    for (int i = 0; i < SSD1306_WIDTH; i++) {
        ssd1306_DrawPixel(i, SCOREBOARD_HEIGHT, White); // Górna krawędź poniżej score board
        ssd1306_DrawPixel(i, SSD1306_HEIGHT - 1, White); // Dolna krawędź
    }
    for (int i = SCOREBOARD_HEIGHT; i < SSD1306_HEIGHT; i++) {
        ssd1306_DrawPixel(0, i, White); // Lewa krawędź
        ssd1306_DrawPixel(SSD1306_WIDTH - 1, i, White); // Prawa krawędź
    }
    ssd1306_UpdateScreen();
}

void generateNewPoint(void) {
    pointX = ((rand() % ((SSD1306_WIDTH / PIXEL_SIZE) - 2)) + 1) * PIXEL_SIZE;
    pointY = (((rand() % ((SSD1306_HEIGHT - SCOREBOARD_HEIGHT) / PIXEL_SIZE - 2)) + 1) * PIXEL_SIZE) + SCOREBOARD_HEIGHT;
}

void drawPoint(void) {
    for (int i = 0; i < PIXEL_SIZE; i++) {
        for (int j = 0; j < PIXEL_SIZE; j++) {
            ssd1306_DrawPixel(pointX + i, pointY + j, White);
        }
    }
}

void updateSnakePosition(void) {
    // Przesuwamy ciało węża
    for (int i = snakeLength - 1; i > 0; i--) {
        snakeX[i] = snakeX[i - 1];
        snakeY[i] = snakeY[i - 1];
    }

    // Aktualizujemy pozycję głowy węża
    snakeX[0] += snakeDirectionX;
    snakeY[0] += snakeDirectionY;

    // Detekcja kolizji z krawędziami
    if (snakeX[0] < 0 || snakeX[0] >= SSD1306_WIDTH || snakeY[0] < SCOREBOARD_HEIGHT || snakeY[0] >= SSD1306_HEIGHT) {
        displayGameOverMessage();
        osDelay(5000); // Wyświetlanie komunikatu przez 5 sekund
        gameRunning = 0; // Ustawienie flagi na 0
        handleLongPress(); // Powrót do menu głównego
    }

    // Detekcja kolizji z samym sobą
    for (int i = 1; i < snakeLength; i++) {
        if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) {
            displayGameOverMessage();
            osDelay(5000); // Wyświetlanie komunikatu przez 5 sekund
            gameRunning = 0; // Ustawienie flagi na 0
            displayMenu(); // Powrót do menu głównego
        }
    }

    // Detekcja zjedzenia punktu
    if (snakeX[0] == pointX && snakeY[0] == pointY) {
        if (snakeLength < MAX_SNAKE_LENGTH) {
            snakeLength++; // Zwiększamy długość węża
        }
        score++; // Zwiększamy wynik
        generateNewPoint(); // Generujemy nowy punkt
    }
}

void drawSnake(void) {
    for (int i = 0; i < snakeLength; i++) {
        for (int x = 0; x < PIXEL_SIZE; x++) {
            for (int y = 0; y < PIXEL_SIZE; y++) {
                ssd1306_DrawPixel(snakeX[i] + x, snakeY[i] + y, White);
            }
        }
    }
    ssd1306_UpdateScreen();
}

void displayScore(void) {
    char buffer[20];
    ssd1306_SetCursor(0, 0);
    sprintf(buffer, "Score: %d", score);
    ssd1306_WriteString(buffer, Font_6x8, White);
    ssd1306_UpdateScreen();
}

void handleInput(void) {
    // Przykład obsługi joysticka (lub innego wejścia użytkownika) do zmiany kierunku ruchu węża
    if (yAxisValue > (4095 - JOY_Y_THRESHOLD) && snakeDirectionY == 0) {
        // Move down
        snakeDirectionX = 0;
        snakeDirectionY = SNAKE_SPEED;
    } else if (yAxisValue < JOY_Y_THRESHOLD && snakeDirectionY == 0) {
        // Move up
        snakeDirectionX = 0;
        snakeDirectionY = -SNAKE_SPEED;
    } else if (xAxisValue > (4095 - JOY_X_THRESHOLD) && snakeDirectionX == 0) {
        // Move left (odwrócone)
        snakeDirectionX = -SNAKE_SPEED;
        snakeDirectionY = 0;
    } else if (xAxisValue < JOY_X_THRESHOLD && snakeDirectionX == 0) {
        // Move right (odwrócone)
        snakeDirectionX = SNAKE_SPEED;
        snakeDirectionY = 0;
    }
}

// Funkcja resetująca stan gry Snake
void Snake_Reset(void) {
    snakeLength = 1;
    snakeX[0] = (SSD1306_WIDTH / 2 / PIXEL_SIZE) * PIXEL_SIZE;
    snakeY[0] = ((SSD1306_HEIGHT / 2 / PIXEL_SIZE) * PIXEL_SIZE) + SCOREBOARD_HEIGHT;
    snakeDirectionX = SNAKE_SPEED;
    snakeDirectionY = 0;
    score = 0; // Resetowanie wyniku
    generateNewPoint(); // Generujemy pierwszy punkt
}

// Funkcja inicjalizacyjna gry Snake
void Snake_Init(void) {
    // Resetowanie stanu gry
    Snake_Reset();

    // Inicjalizacja zasobów potrzebnych do gry
    // Utworzenie zadania FreeRTOS dla gry Snake
    const osThreadAttr_t snakeTaskAttr = {
        .name = "snakeTask",
        .priority = osPriorityNormal,
        .stack_size = 512 // odpowiedni rozmiar stosu dla gry
    };
    snakeTaskHandle = osThreadNew(Snake_Run, NULL, &snakeTaskAttr);
}

// Funkcja wyświetlająca Game Over i wynik
void displayGameOverMessage(void) {
    osMutexAcquire(oledMutexHandle, osWaitForever);
    ssd1306_Fill(Black); // Clear screen

    const char* gameOverMessage = "Game Over";
    char scoreMessage[20];

    sprintf(scoreMessage, "Score: %d", score);

    // Center "Game Over" message
    ssd1306_SetCursor((SSD1306_WIDTH - (strlen(gameOverMessage) * 6)) / 2, SSD1306_HEIGHT / 2 - 10);
    ssd1306_WriteString(gameOverMessage, Font_6x8, White);

    // Center score message
    ssd1306_SetCursor((SSD1306_WIDTH - (strlen(scoreMessage) * 6)) / 2, SSD1306_HEIGHT / 2 + 2);
    ssd1306_WriteString(scoreMessage, Font_6x8, White);

    ssd1306_UpdateScreen(); // Refresh OLED display
    osMutexRelease(oledMutexHandle);
}

// Główna pętla gry Snake
void Snake_Run(void *argument) {
    displayMessage("Starting Snake...");
    osDelay(100); // Tymczasowe opóźnienie dla demonstracji

    Snake_Reset(); // Reset stanu gry przed rozpoczęciem pętli gry

    gameRunning = 1; // Ustawienie flagi na 1, oznaczającej uruchomienie gry

    int directionSet = 0; // Flaga kontrolująca, czy kierunek został ustawiony

    for (;;) {
        if (longPressDetected) {
            break;
        }

        handleInput(); // Obsługa wejść użytkownika

        // Sprawdź, czy kierunek został ustawiony
        if (snakeDirectionX != 0 || snakeDirectionY != 0) {
            directionSet = 1; // Kierunek ustawiony
        }

        // Jeśli kierunek został ustawiony, wykonaj ruch i rysuj węża
        if (directionSet) {
            updateSnakePosition(); // Aktualizacja pozycji węża
            drawBorders(); // Rysowanie krawędzi
            drawPoint(); // Rysowanie punktu
            drawSnake(); // Rysowanie węża
            displayScore(); // Wyświetlanie wyniku
        }

        osDelay(100); // Tymczasowe opóźnienie dla demonstracji
    }

    // Jeśli długie naciśnięcie zostało wykryte, wracamy do menu głównego
    osThreadExit();
}
