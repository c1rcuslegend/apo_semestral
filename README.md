# SPACE INVADERS: MICRO EDITION
**Course: APO - Semestral Project**

**Authors: Illia Volkov and Veronika Ihnashkina**

## INSTALLATION 

git clone https://github.com/c1rcuslegend/apo_semestral.git

cd apo_semestral

make

./space_invaders

## PROJECT STRUCTRURE

- `space_invaders.c` - Program entry point that initializes hardware, manages game loop, and handles main state transitions
- `game.c` - Contains the main game loop and state management, including game start, pause, and end conditions
- `game_utils.c` - Implements core game mechanics including enemy movement, bullet handling, collision detection, and scoring, which are used in the game loop (game.c)
- `main_menu.c` - Implements the main menu system with selection handling and high score display
- `gui.c` - Displays user interface elements like start menu, game over screen, and settings menus
- `ppm_image.c` - Handles loading and rendering sprite images from PPM format files with transparency support
- `input.c` - Processes player input from knobs (rotation and button presses) and manages LED indicators
- `graphics.c` - Provides drawing primitives for pixels, characters, strings, and screen updates
- `settings.c` - Game settings wrapper for managing the game configuration
- `texter.c` - Handles text writing to and reading from the text file for saving high scores

These files together create a Space Invaders ([Wikipedia](https://en.wikipedia.org/wiki/Space_Invaders)) game designed for embedded hardware with physical knob controls, LED indicators, and an LCD display.

## [WIKI](https://github.com/c1rcuslegend/apo_semestral/wiki/User-Manual) 
