# Solix Breakout Example

A complete, cross-platform 2D Breakout arcade game built in the [Solix programming language](https://github.com/your-username/solix) using [Raylib](https://www.raylib.com/).

## Features

- **Start / Title Screen**: Game instructions and highest score tracker.
- **Classic Gameplay**:
  - 5 rows of 10 color-coded bricks with distinct score point tiers.
  - Realistic angle reflection based on paddle hit position.
  - 3 lives with on-screen HUD (Score, Lives, Bricks remaining, High Score).
- **Pause Menu**: Press `P` anytime during gameplay to pause, with options to resume, restart (`R`), or return to main menu (`M`).
- **End Game Screens**:
  - **Game Over**: Shows your final score and highest score, with quick restart (`ENTER`/`SPACE`) or menu navigation.
  - **Victory**: Unlocked by clearing all 50 bricks! Awards remaining life bonus points.
- **Cross-Platform**: Uses Solix VM FFI and Raylib native library.

## Controls

| Key | Action |
| --- | --- |
| `LEFT` / `A` | Move paddle left |
| `RIGHT` / `D` | Move paddle right |
| `SPACE` | Launch ball / Select option |
| `ENTER` | Start game / Try again |
| `P` | Pause / Unpause game |
| `R` | Restart game (while paused) |
| `M` | Return to main menu (from pause or game over) |
| `ESC` | Quit game |

## Running the Game

To build and run the example:

```bash
# From anywhere
solix run /path/to/solix/exemple

# Or from inside this directory
cd exemple
solix run .
```
