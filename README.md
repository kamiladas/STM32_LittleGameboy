
# STM32 Little Gameboy Project

## Overview
The STM32 Little Gameboy is a compact gaming device built using the STM32 microcontroller and an OLED screen. The device is designed to play classic games such as Snake and Pong, with user input handled through a joystick controller. This project showcases the capabilities of the STM32 microcontroller in handling graphical output and user input to create a simple yet entertaining gaming experience.

## Features
- **Microcontroller**: STM32F407VGTX
- **Display**: 128x64 OLED screen
- **Input**: Joystick for navigation and game control
- **Games Included**: Snake, Pong
- **Compact Design**: Portable and lightweight for on-the-go gaming

## Hardware Components
- **STM32F407VGTX Development Board**
- **128x64 OLED Display**
- **Analog Joystick Module**
- **Breadboard and Connecting Wires**
- **Power Supply (Battery or USB)**

## Software and Tools
- **STM32CubeMX**: For generating initialization code and configuring peripherals
- **STM32CubeIDE**: Integrated development environment for coding, debugging, and compiling
- **HAL Library**: Hardware abstraction layer for STM32
- **ST-Link Utility**: For programming and debugging the STM32 microcontroller

## Project Description
The STM32 Little Gameboy project involves creating a simple gaming console that uses an OLED screen to display game graphics and a joystick to control the gameplay. The device supports two classic games: Snake and Pong. Here's a detailed breakdown of the project:

### Hardware Setup:
- Connect the OLED screen to the STM32 development board using I2C or SPI communication.
- Connect the joystick module to the analog pins of the STM32 for reading directional inputs.
- Power the setup using a battery pack or USB connection.

### Software Development:
- **Initialization**: Use STM32CubeMX to configure the microcontroller's peripherals, including I2C/SPI for the OLED screen and ADC for the joystick.
- **Graphics Library**: Implement or integrate a graphics library to handle drawing operations on the OLED screen.
- **Game Logic**: Develop the game logic for Snake and Pong, ensuring smooth gameplay and responsive controls.
  - **Snake**: Implement the classic Snake game where the player controls a snake to eat food and grow in length, avoiding collisions with the walls and itself.
  - **Pong**: Implement a simple Pong game where the player controls a paddle to bounce a ball and score points.
- **Input Handling**: Read joystick inputs and map them to game controls for smooth and intuitive gameplay.

### Compilation and Programming:
- Compile the project using STM32CubeIDE.
- Program the STM32 microcontroller using the ST-Link utility or integrated tools within STM32CubeIDE.

## How to Build and Run
1. **Clone the Repository**:
   ```sh
   git clone https://github.com/your-username/STM32_LittleGameboy.git
   ```
2. **Open the Project** in STM32CubeIDE and generate the code using STM32CubeMX.
3. **Build the Project** within STM32CubeIDE.
4. **Upload the Firmware** to the STM32 microcontroller using the ST-Link debugger.
5. **Power Up the Device** and start playing the games using the joystick for control.

## Future Enhancements
- Add more classic games such as Tetris, Breakout, and Space Invaders.
- Implement sound effects using a small speaker and PWM output.
- Enhance the graphical interface with more detailed sprites and animations.
- Improve power efficiency for longer battery life.

## Example Video


https://github.com/user-attachments/assets/83f3c389-71e0-4f83-badb-b8de8ea42ac7



## Conclusion
The STM32 Little Gameboy project is an excellent way to explore the capabilities of the STM32 microcontroller and OLED displays. It provides a fun and interactive way to learn about embedded systems, game development, and hardware interfacing. Whether you're a beginner or an experienced developer, this project offers a rewarding experience in creating a portable gaming device.
