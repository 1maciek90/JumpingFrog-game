# JumpingFrog Game

A simple console-based game where you control a frog to jump and avoid obstacles. This project is written in C and utilizes the [PDCurses](https://pdcurses.org/) library for cross-platform console graphics.

## Features

- Classic arcade-style gameplay
- Console-based interface using curses
- Cross-platform (Windows, Linux, macOS) via PDCurses

## Requirements

- **C Compiler** (e.g., gcc, clang)
- **PDCurses library**

## Installation

### 1. Install PDCurses

You must have the PDCurses library installed on your system.  
Visit [PDCurses downloads](https://pdcurses.org/) for installation instructions specific to your platform.

#### On Linux (example for Ubuntu/Debian):

```bash
sudo apt-get install libncurses-dev
# or build PDCurses from source if you want the PDCurses version specifically
```

#### On Windows:

Download or build PDCurses from [https://pdcurses.org/](https://pdcurses.org/), and follow their build instructions.

### 2. Clone the repository

```bash
git clone https://github.com/1maciek90/JumpingFrog-game.git
cd JumpingFrog-game
```

### 3. Build the project

Adjust the compilation command if your PDCurses library is in a custom location.

```bash
gcc -o jumpingfrog main.c -lpdcurses
```

or, for some systems:

```bash
gcc -o jumpingfrog main.c -lcurses
```

## Usage

Run the compiled executable:

```bash
./jumpingfrog
```

Follow the on-screen instructions to play.

## Controls

- Arrow keys: Move/jump the frog
- Q: Quit the game

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

## Credits

- [PDCurses](https://pdcurses.org/) for the console graphics library.

---

Happy jumping!
