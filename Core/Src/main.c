#include "main.h"
#include "cmsis_os2.h"
#include "i2c.h"
#include "gpio.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "adc.h"
#include "snake.h" // Dodanie pliku nagłówkowego gry Snake
#include <string.h> // Dodanie nagłówka dla strlen

// Define joystick threshold values
#define JOY_THRESHOLD 300
#define JOY_X_THRESHOLD 300
#define JOY_Y_THRESHOLD 300

// Define OLED display width and height
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64

// Define states for the menu and games
typedef enum {
    MENU_MAIN,
    MENU_TETRIS,
    MENU_SNAKE,
    MENU_PONG,
    MENU_COUNT
} MenuState;

MenuState currentMenu = MENU_MAIN;
int menuIndex = 1; // Start from the first game option, skipping "Main Menu"

// Flag to indicate if a long press has occurred
volatile uint8_t longPressDetected = 0;
volatile uint8_t ignoreNextPress = 0; // Flag to ignore the next button press
volatile uint8_t gameRunning = 0; // Flag to indicate if a game is running

// FreeRTOS task handles
osThreadId_t oledTaskHandle;
osThreadId_t joystickTaskHandle;
osThreadId_t longPressTaskHandle;

// Mutex handle for OLED display
osMutexId_t oledMutexHandle;
osMutexAttr_t oledMutex_attr = {
    .name = "oledMutex",
    .attr_bits = osMutexRecursive
};

// Function prototypes
void SystemClock_Config(void);
void StartOledTask(void *argument);
void StartJoystickTask(void *argument);
void StartLongPressTask(void *argument);
void readJoystick(void);
void updateMenu(void);
void displayMenu(void);
void displayMessage(const char* message);
void handleLongPress(void);
void startGame(MenuState game);
void startTetris(void);
void startPong(void);

// Joystick ADC values
uint32_t yAxisValue = 0;
uint32_t xAxisValue = 0;

int main(void) {
    // Initialize HAL Library
    HAL_Init();

    // Configure the system clock
    SystemClock_Config();

    // Initialize all configured peripherals
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_ADC1_Init(); // Initialize ADC for joystick

    // Initialize FreeRTOS Kernel
    osKernelInitialize();

    // Create mutex for OLED display
    oledMutexHandle = osMutexNew(&oledMutex_attr);

    // Create tasks
    oledTaskHandle = osThreadNew(StartOledTask, NULL, NULL);
    joystickTaskHandle = osThreadNew(StartJoystickTask, NULL, NULL);
    longPressTaskHandle = osThreadNew(StartLongPressTask, NULL, NULL);

    // Start FreeRTOS Scheduler
    osKernelStart();
    displayMenu();

    ssd1306_UpdateScreen();
    // Main loop
    while (1) {
    }
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // Configure power voltage scaling
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    // Configure the main internal regulator output voltage
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    // Configure the system clock
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
        Error_Handler();
    }
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {
    }
}

// Read joystick values
void readJoystick(void) {
    ADC_ChannelConfTypeDef sConfig = {0};

    // Configure for Y-axis (PA0 - ADC_CHANNEL_0)
    sConfig.Channel = ADC_CHANNEL_0; // Y-axis
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        Error_Handler();
    }
    HAL_ADC_Start(&hadc1); // Start ADC conversion for Y-axis
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    yAxisValue = HAL_ADC_GetValue(&hadc1); // Read Y-axis value

    // Configure for X-axis (PA1 - ADC_CHANNEL_1)
    sConfig.Channel = ADC_CHANNEL_1; // X-axis
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        Error_Handler();
    }
    HAL_ADC_Start(&hadc1); // Start ADC conversion for X-axis
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    xAxisValue = HAL_ADC_GetValue(&hadc1); // Read X-axis value
}

// Update menu based on joystick input
void updateMenu(void) {
    // Read joystick values
    readJoystick();

    // Handle Y-axis (up and down) movements
    if (yAxisValue > (4095 - JOY_Y_THRESHOLD)) {
        // Move down
        if (menuIndex < MENU_COUNT - 1) {
            menuIndex++;
            displayMenu();
        }
        osDelay(500); // Debounce delay
    } else if (yAxisValue < JOY_Y_THRESHOLD) {
        // Move up
        if (menuIndex > 1) { // Skip "Main Menu"
            menuIndex--;
            displayMenu();
        }
        osDelay(500); // Debounce delay
    }

    // Handle button press (GPIOC Pin 7 as pull-up)
    if (ignoreNextPress) {
        // Ignore this press and reset the flag
        if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_7) == GPIO_PIN_SET) { // Button released
            ignoreNextPress = 0; // Reset the flag
        }
    } else {
        if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_7) == GPIO_PIN_RESET) { // Button pressed
            currentMenu = (MenuState)menuIndex;
            osDelay(500); // Debounce delay
            startGame(currentMenu);
        }
    }

    // Delay to prevent excessive CPU usage
    osDelay(50);
}

// Display a message on the OLED screen
void displayMessage(const char* message) {
    osMutexAcquire(oledMutexHandle, osWaitForever);
    ssd1306_Fill(Black); // Clear screen
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString(message, Font_6x8, White);
    ssd1306_UpdateScreen(); // Refresh OLED display
    osMutexRelease(oledMutexHandle);
}

// Display menu on OLED
void displayMenu(void) {
    if (gameRunning) {
        return; // Do not display menu if game is running
    }

    osMutexAcquire(oledMutexHandle, osWaitForever);
    ssd1306_Fill(Black); // Clear screen

    const char* menuItems[] = {
        "Main Menu",
        "Tetris",
        "Snake",
        "Pong"
    };

    // Center "Main Menu" title
    ssd1306_SetCursor((SSD1306_WIDTH - (strlen(menuItems[0]) * 6)) / 2, 0);
    ssd1306_WriteString(menuItems[0], Font_6x8, White);

    for (int i = 1; i < MENU_COUNT; i++) {
        ssd1306_SetCursor(0, (i + 1) * 10);
        if (i == menuIndex) {
            ssd1306_WriteString("> ", Font_6x8, White); // Highlight current selection
        } else {
            ssd1306_WriteString("  ", Font_6x8, White);
        }
        ssd1306_WriteString(menuItems[i], Font_6x8, White);
    }

    ssd1306_UpdateScreen(); // Refresh OLED display
    osMutexRelease(oledMutexHandle);
}

// Handle long press to return to main menu
void handleLongPress(void) {
    longPressDetected = 1;
    ignoreNextPress = 1; // Set flag to ignore the next button press
    gameRunning = 0;
    currentMenu = MENU_MAIN;
    menuIndex = 1; // Reset to the first selectable item
    displayMenu();
}

// Long press detection task
void StartLongPressTask(void *argument) {
    for (;;) {
        if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_7) == GPIO_PIN_RESET) { // Button pressed
            osDelay(1000); // Wait for 1 second
            if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_7) == GPIO_PIN_RESET) { // Still pressed -> long press
                handleLongPress();
            }
        }
        osDelay(100); // Small delay to reduce CPU load
    }
}

// Start the selected game
void startGame(MenuState game) {
    longPressDetected = 0; // Reset long press flag
    gameRunning = 1; // Set the game flag to running
    switch (game) {
        case MENU_TETRIS:
            startTetris();

            break;
        case MENU_SNAKE:
            Snake_Init();

            break;
        case MENU_PONG:
            startPong();
            break;
        default:
            break;
    }

    if(!gameRunning){ displayMenu();} // Display main menu after the game ends
}

// Placeholder for Tetris game
void startTetris(void) {
    displayMessage("Starting Tetris...");
    // Implement Tetris game logic here
    while (!longPressDetected) {
        // Tetris game loop
        osDelay(100); // Adjust delay as necessary
    }
    handleLongPress(); // Return to main menu if long press detected
}

// Placeholder for Pong game
void startPong(void) {
    displayMessage("Starting Pong...");
    // Implement Pong game logic here
    while (!longPressDetected) {
        // Pong game loop
        osDelay(100); // Adjust delay as necessary
    }
    handleLongPress(); // Return to main menu if long press detected
}

// Task for OLED display
void StartOledTask(void *argument) {
    ssd1306_Init(); // Initialize OLED display

    for (;;) {
        if (!gameRunning) { // Only when no game is running
            displayMenu(); // Display the current menu
        }
        osDelay(100); // Small delay for stability
    }
}

// Task for joystick input
void StartJoystickTask(void *argument) {
    for (;;) {
        readJoystick(); // Read joystick values

        // Only update menu if no game is running
        if (!gameRunning) {
            updateMenu(); // Update menu based on joystick input
        }

        osDelay(50); // Small delay to reduce CPU load
    }
}
