#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <curses.h>
#include <cstdlib>
#include <time.h>

using namespace std;

// COLORS
#define MAIN_COLOR 1
#define FROG_COLOR 2
#define GRASS_COLOR 3
#define DESTINATION_COLOR 4
#define ROAD_COLOR 5
#define CARM_COLOR 6
#define CARS_COLOR 7
#define CARC_COLOR 8
#define OBSTACLE_COLOR 9
#define COIN_COLOR 10
#define STORK_COLOR 11

// BOARD SETUP
#define X 0
#define Y 0
#define COINS_COUNT 10
#define COIN_SIGN '*'
#define OBSTACLE_COUNT 30
#define OBSTACLE_SIGN 'X'

// STATS SETUP
#define STATS_WIDTH 20
#define STATS_HEIGHT 9

// STORK_SETUP
#define STORK_SIGN 'S'
#define TIME_TO_STORK 5

// FROG SETUP
#define FROG_REMAINING_MOVES 5

// CAR SETUP
#define CAR_MAX_DISTANCE_FROM_FROG 2


// POSITION TYPES
#define GRASS 0
#define ROAD 1
#define DESTINATION 2
#define OBSTACLE 3
#define COIN 4

// TIMER SETUP
#define START_TIME 0
#define CHECK_FRAME_RATE 0
#define FRAME_RATE 100
#define MAX_FRAME_RATE 1000




//------------------------------------------------
//----------------  DATA STRUCTURES --------------
//------------------------------------------------

struct WIN {
    WINDOW* window;
    int x, y;
    int width, height;
    int color;
};

struct CAR {
    WIN* win;
    int x;
    int y;
    int length;
    int direction;
    int color;
    int speed;
    int xSpeedChange;
    int alwaysMove;
    char sign;
};

struct FROG {
    WIN* win;
    int x;
    int y;
    int remainingMoves;
    int points;
    int maxMoves;
    int color;
    char sign;
    bool carried;
    clock_t lastMoveTime;
    CAR* carringCar;
};

struct STORK {
    WIN* win;
    int x;
    int y;
    int color;
    char sign;
    int timeToStork;
    clock_t lastMoveTime;
};

struct TIMER {
    int time;
    int carsTiming;
    int carsTime;
    int frameRate;
    int maxFrameRate;
    int checkFrameRate;
};

//------------------------------------------------
//----------------  WINDOW FUNCTIONS -------------
//------------------------------------------------

WINDOW* Start() {
    WINDOW* mainwin;
    if ((mainwin = initscr()) == NULL) {
        cerr << "Error initialising ncurses." << endl;      // Init ncurses
        exit(EXIT_FAILURE);
    }

    start_color();                                      // Init colors
    init_pair(MAIN_COLOR, COLOR_WHITE, COLOR_BLACK);
    init_pair(FROG_COLOR, COLOR_GREEN, COLOR_BLACK);
    init_pair(CARM_COLOR, COLOR_RED, COLOR_WHITE);
    init_pair(CARS_COLOR, COLOR_BLUE, COLOR_WHITE);
    init_pair(CARC_COLOR, COLOR_YELLOW, COLOR_WHITE);
    init_pair(GRASS_COLOR, COLOR_BLACK, COLOR_GREEN);
    init_pair(DESTINATION_COLOR, COLOR_BLACK, COLOR_YELLOW);
    init_pair(ROAD_COLOR, COLOR_BLACK, COLOR_WHITE);
    init_pair(OBSTACLE_COLOR, COLOR_RED, COLOR_GREEN);
    init_pair(COIN_COLOR, COLOR_YELLOW, COLOR_GREEN);
    init_pair(STORK_COLOR, COLOR_CYAN, COLOR_BLACK);

    noecho();
    curs_set(0);
    return mainwin;
}

//--------------  MENU FUNCTION ------------- 

void Welcome(WINDOW* mainwin) {
    int choice;
    int highlight = 0;

    const char* choices[] = {               // Setup options
        "Start Game",
        "Instructions",
        "Exit"
    };
    int n_choices = sizeof(choices) / sizeof(char*);

    keypad(mainwin, TRUE);
    while (true) {
        for (int i = 0; i < n_choices; ++i) {
            if (i == highlight) {
                wattron(mainwin, A_REVERSE);
            }
            mvwprintw(mainwin, i + 5, 5, choices[i]);
            wattroff(mainwin, A_REVERSE);
        }
        choice = wgetch(mainwin);

        switch (choice) {
        case KEY_UP:
            highlight = (highlight - 1 + n_choices) % n_choices;
            break;
        case KEY_DOWN:
            highlight = (highlight + 1) % n_choices;
            break;
        case 10:
            if (highlight == 0) {       // Start the game
                wclear(mainwin);
                wrefresh(mainwin);
                return;
            }
            else if (highlight == 1) {      // Instructions
                wclear(mainwin);
                mvwprintw(mainwin, 5, 5, "Instructions:");
                mvwprintw(mainwin, 7, 5, "Use arrows keys to move the frog.");
                mvwprintw(mainwin, 9, 5, "Avoid cars and the stork and reach the other side of the road.");
                mvwprintw(mainwin, 11, 5, "Press any key to return to the menu ...");
                wrefresh(mainwin);
                wgetch(mainwin);
            }
            else if (highlight == 2) {        // Exit the game
                endwin();
                exit(0);
            }
            break;
        default:
            break;
        }
        wclear(mainwin);
    }
}

//------------------------------------------------
//----------------  WIN FUNCTIONS ----------------
//------------------------------------------------

WIN* Init(WINDOW* parent, int rows, int cols, int y, int x, int color) {
    WIN* w = new WIN;
    w->x = x;
    w->y = y;
    w->width = cols;
    w->height = rows;
    w->color = color;
    w->window = subwin(parent, w->height, w->width, w->y, w->x);
    wbkgd(w->window, COLOR_PAIR(w->color));
    wrefresh(w->window);
    return w;
}

//------------------------------------------------
//----------- ROADS & CARS FUNCTIONS -------------
//------------------------------------------------

void InitCars(WIN* win, CAR cars[], int roadPositions[], int carsAndRoadsCount, int carLength, int carSpeed, int carMovingColor, int carStopColor, char carSign) {
    for (int i = 0; i < carsAndRoadsCount; i++) {
        cars[i].win = win;
        cars[i].y = roadPositions[i];
        cars[i].x = rand() % (win->width - 2 - carLength) + 1;
        cars[i].length = rand() % carLength + 1;
        cars[i].direction = (rand() % 2 == 0) ? 1 : -1;
        cars[i].speed = rand() % carSpeed + 1;
        cars[i].xSpeedChange = rand() % (win->width - 2 - carLength) + 1;
        cars[i].alwaysMove = rand() % 2;
        if (cars[i].alwaysMove == 1) cars[i].color = carMovingColor;
        else  cars[i].color = carStopColor;
        cars[i].sign = carSign;
    }
}

bool CheckIfFrogIsOnCar(FROG* frog, CAR car) {
    if (frog->y == car.y) {                         // Check if the frog is on the car
        for (int j = 0; j < car.length; j++) {
            if (frog->x == car.x + j * car.direction) {
                return true;
            }
        }
    }
    return false;
}

bool CheckIfFrogIsClose(FROG* frog, CAR* car) {
    if (car->alwaysMove == 1) {
        return true;
    }
    // Check if the frog is close to the car
    for (int i = 0; i < car->length; i++) {
        if (frog->y == car->y && abs(frog->x - (car->x + i * car->direction)) <= CAR_MAX_DISTANCE_FROM_FROG) {
            return false;
        }
    }
    return true;
}

void ResetCar(FROG* frog, CAR* car, int width, int y, int carLength, int carSpeed, int carMovingColor, int carStopColor) {
    if (frog->carringCar != nullptr && car == frog->carringCar) {
        car->speed = rand() % carSpeed + 1;                         // The same car when the frog is inside
        car->xSpeedChange = rand() % (width - 2 - carLength) + 1;
    }
    else {
        car->length = rand() % carLength + 1;
        car->speed = rand() % carSpeed + 1;
        car->xSpeedChange = rand() % (width - 2 - carLength) + 1;       // New cars with new parameters when frog is outside
        car->alwaysMove = rand() % 2;
        if (car->alwaysMove == 1) car->color = carMovingColor;
        else  car->color = carStopColor;
    }
    car->x = (car->direction == 1) ? 0 - car->length : width + car->length - 1;     // Change the position of the car (depends on car direction)
}

void DrawCars(CAR* car) {
    wattron(car->win->window, COLOR_PAIR(car->color));
    for (int i = 0; i < car->length; i++) {
        int newX = car->x + i * car->direction;             // Draw the car on the new position (depends on car direction)
        if (newX > 0 && newX < car->win->width - 1) {
            mvwaddch(car->win->window, car->y, newX, car->sign);
        }
    }

    wattroff(car->win->window, COLOR_PAIR(car->color));
}

void EraseCars(CAR* car, int** positionType) {
    for (int i = 0; i < car->length; i++) {
        int newX = car->x + i * car->direction;             // Erase the last position of the car
        if (newX > 0 && newX < car->win->width - 1) {
            wattron(car->win->window, COLOR_PAIR(ROAD_COLOR));
            mvwaddch(car->win->window, car->y, newX, ' ');
            wattroff(car->win->window, COLOR_PAIR(ROAD_COLOR));
        }
    }
}

void MoveCars(CAR cars[], TIMER* timer, FROG* frog, int** positionType, int carsAndRoadsCount, int carLength, int carSpeed, int carMoveColor, int carStopColor) {
    for (int i = 0; i < carsAndRoadsCount; i++) {
        EraseCars(&cars[i], positionType);    // Erase the cars
        if (timer->carsTime % cars[i].speed == 0 && CheckIfFrogIsClose(frog, &cars[i])) {       // Move cars if the time is right
            cars[i].x += cars[i].direction;                                                     // and frog far enough from the friendly cars
        }
        if (cars[i].xSpeedChange == cars[i].x) {                    // Change the speed of the car during the game
            cars[i].speed = rand() % carSpeed + 1;
        }

        if (frog->carried && frog->carringCar == &cars[i] && timer->carsTime % frog->carringCar->speed == 0) {
            frog->carringCar->x += cars[i].direction;               // Move the car when the frog inside   
            frog->x = frog->carringCar->x;
        }

        if ((cars[i].direction == 1 && cars[i].x - cars[i].length > cars[i].win->width - 2)
            || (cars[i].direction == -1 && cars[i].x < 1 - cars[i].length)) {
            ResetCar(frog, &cars[i], cars[i].win->width, cars[i].y, carLength, carSpeed, carMoveColor, carStopColor);       // New cars after they leave the border
        }
        DrawCars(&cars[i]);     // Draw the cars
    }
}

bool CarColision(FROG* frog, CAR cars[], TIMER* timer, int carsAndRoadsCount) {
    for (int i = 0; i < carsAndRoadsCount; i++) {
        if (frog->y == cars[i].y && cars[i].alwaysMove != 0) {          // Check collisions with red (enemy) cars
            for (int j = 0; j < cars[i].length; j++) {
                if (frog->x == cars[i].x + j * cars[i].direction) {
                    return true;
                }
            }
        }
    }
    return false;
}

//------------------------------------------------
//----------------  FROG FUNCTIONS ---------------
//------------------------------------------------

bool CheckFrogStartPosition(int** positionType, int x, int y) {
    if (positionType[y][x] == OBSTACLE || positionType[y][x] == COIN) {     // Frog can't start from obstacle or coin
        return false;
    }
    return true;
}

FROG* InitFrog(WIN* w, int frogColor, char frogSign, int maxMoves, int** positionType) {
    FROG* frog = new FROG;
    frog->win = w;
    frog->x = rand() % (w->width - 2) + 1;
    frog->y = w->height - 2;
    while (!CheckFrogStartPosition(positionType, frog->x, frog->y)) {
        frog->x = rand() % (w->width - 2) + 1;
    }
    frog->points = 0;
    frog->remainingMoves = maxMoves;
    frog->maxMoves = maxMoves;
    frog->color = frogColor;
    frog->sign = frogSign;
    frog->carried = false;
    frog->carringCar = nullptr;
    return frog;
}

void DrawFrog(FROG* frog) {
    wattron(frog->win->window, COLOR_PAIR(frog->color));        // Draw frog
    mvwaddch(frog->win->window, frog->y, frog->x, frog->sign);
    wattroff(frog->win->window, COLOR_PAIR(frog->color));
    wrefresh(frog->win->window);
}

void EraseFrog(FROG* frog, int** positionType) {
    int mapY = frog->y;
    int mapX = frog->x;

    int color = 0;
    char ch = ' ';
    switch (positionType[mapY][mapX]) {
    case GRASS:     // Grass
        color = GRASS_COLOR;
        break;
    case ROAD:      // Road
        color = ROAD_COLOR;
        break;
    case DESTINATION:       // Destination
        color = DESTINATION_COLOR;
        break;
    case OBSTACLE:      // Obstacle
        color = OBSTACLE_COLOR;
        ch = OBSTACLE_SIGN;
        break;
    case COIN:          // Coin
        color = GRASS_COLOR;
        break;
    }

    wattron(frog->win->window, COLOR_PAIR(color));
    mvwaddch(frog->win->window, mapY, mapX, ch);
    wattroff(frog->win->window, COLOR_PAIR(color));
    wrefresh(frog->win->window);
}

bool CanMove(clock_t lastMoveTime, double interval) {
    clock_t currentTime = clock();
    double timeDiff = double(currentTime - lastMoveTime) / CLOCKS_PER_SEC;
    return (timeDiff >= interval);      // True if timeDiff >= interval
}

void CheckFrogMove(WIN* playwin, FROG* frog, int** positionType, int newX, int newY) {
    if (newX > 0 && newX < playwin->width - 1 && newY > 0 && newY < playwin->height - 1) {
        if (positionType[newY][newX] != OBSTACLE) {
            if (positionType[newY][newX] == COIN) {         // Check the type of the new position
                positionType[newY][newX] = GRASS;           // and allow or block the move
                frog->points++;
            }
            EraseFrog(frog, positionType);
            frog->x = newX;
            frog->y = newY;
            frog->remainingMoves--;             // Change the parameters of the frog and stats if the move is possible
            frog->lastMoveTime = clock();
            DrawFrog(frog);
        }
    }
}

void FrogAndCarInteraction(WIN* playwin, FROG* frog, CAR cars[], int carsAndRoadsCount, int carStopColor, int carFrogColor) {
    if (frog->carried && frog->x > 0 && frog->x <= playwin->width - 2) {
        frog->carringCar->color = carStopColor;
        frog->carried = false;
        frog->carringCar = nullptr;             // Frog exit the car
        DrawFrog(frog);
        return;
    }
    else if (!frog->carried) {
        for (int i = 0; i < carsAndRoadsCount; i++) {
            if (CheckIfFrogIsOnCar(frog, cars[i])) {            // Frog enetr the car
                frog->carried = true;
                frog->carringCar = &cars[i];
                frog->carringCar->color = carFrogColor;
            }
        }
        return;
    }
}

void FrogMovement(WIN* playwin, FROG* frog, CAR cars[], int carsAndRoadsCount, int** positionType, int carStopColor, int carFrogColor) {
    int ch = wgetch(playwin->window);

    if (!CanMove(frog->lastMoveTime, 0.2)) {        // Break between moves
        return;
    }

    if (ch == ' ') {
        FrogAndCarInteraction(playwin, frog, cars, carsAndRoadsCount, carStopColor, carFrogColor);      // Friendly car interaction (blue one)
        return;
    }

    int newX = frog->x;
    int newY = frog->y;
    if (frog->remainingMoves > 0 && frog->carried == false) {
        switch (ch) {
        case KEY_UP:                    // Moving frog in 4 directions
            newY--;
            break;
        case KEY_DOWN:
            newY++;
            break;
        case KEY_LEFT:
            newX--;
            break;
        case KEY_RIGHT:
            newX++;
            break;
        default:
            return;
        }
        CheckFrogMove(playwin, frog, positionType, newX, newY);  // Check if frog can move on the new place   
    }

    while (wgetch(playwin->window) != ERR) {          // Clear buffer
        NULL;
    }
}


//------------------------------------------------
//---------------  STORK FUNCTIONS ---------------
//------------------------------------------------

STORK* InitStork(WIN* w, int storkColor, char storkSign, int timeToStork) {
    STORK* stork = new STORK;
    stork->win = w;
    stork->x = rand() % (w->width - 2) + 1;
    stork->y = w->height - 2;
    stork->color = storkColor;
    stork->sign = storkSign;
    stork->timeToStork = timeToStork;
    stork->lastMoveTime = clock();
    return stork;
}

void EraseStork(STORK* stork, int** positionType) {
    int mapY = stork->y;
    int mapX = stork->x;

    int color = 0;
    char ch = ' ';
    switch (positionType[mapY][mapX]) {     // Erase the last position of the stork
    case GRASS:     // Grass                // depends on the type of the position
        color = GRASS_COLOR;
        break;
    case ROAD:      // Road
        color = ROAD_COLOR;
        break;
    case DESTINATION:    // Destination
        color = DESTINATION_COLOR;
        break;
    case OBSTACLE:         // Obstacle
        color = OBSTACLE_COLOR;
        ch = OBSTACLE_SIGN;
        break;
    case COIN:    // Coin
        color = COIN_COLOR;
        ch = COIN_SIGN;
        break;
    }

    wattron(stork->win->window, COLOR_PAIR(color));
    mvwaddch(stork->win->window, mapY, mapX, ch);
    wattroff(stork->win->window, COLOR_PAIR(color));
    wrefresh(stork->win->window);
}

void DrawStork(STORK* stork) {
    wattron(stork->win->window, COLOR_PAIR(stork->color));
    mvwaddch(stork->win->window, stork->y, stork->x, stork->sign);      // Draw stork in the new position
    wattroff(stork->win->window, COLOR_PAIR(stork->color));
    wrefresh(stork->win->window);
}

void MoveStork(STORK* stork, FROG* frog, TIMER* timer, int** positionType) {
    if (timer->time < stork->timeToStork) {
        return;                     // Start moving stork after the delay
    }
    else if (timer->time == 5) {
        DrawStork(stork);       // Start drawing stork after 5 seconds
        return;
    }

    clock_t currentTime = clock();
    double timeDiff = double(currentTime - stork->lastMoveTime) / CLOCKS_PER_SEC;

    DrawStork(stork);

    if (timeDiff < 2) {
        return;         // Strork moves every 2 seconds
    }

    EraseStork(stork, positionType);    // Erase the last position of the stork

    if (stork->x < frog->x) {
        stork->x++;
    }                                       // Stork moves behind the frog (vertical, horizontal and diagonal movement)
    else if (stork->x > frog->x) {
        stork->x--;
    }

    if (stork->y < frog->y) {
        stork->y++;
    }
    else if (stork->y > frog->y) {
        stork->y--;
    }

    DrawStork(stork);    // Draw the stork in the new position

    stork->lastMoveTime = currentTime;      // Update the last move time

}


bool StorkColision(FROG* frog, STORK* stork, TIMER* timer) {
    if (timer->time < stork->timeToStork) {         // Start checking collision after 5 seconds
        return false;
    }

    if (frog->y == stork->y && frog->x == stork->x && frog->carried == false) {         // Check colision with stork
        return true;
    }
    return false;
}

//------------------------------------------------
//---------------  TIMER FUNCTIONS ---------------
//------------------------------------------------

TIMER* InitTimer(WIN* w, int startTime, int frameRate, int maxFrameRate, int checkFrameRate, int carTime) {
    TIMER* timer = new TIMER;
    timer->time = startTime;
    timer->frameRate = frameRate;
    timer->maxFrameRate = maxFrameRate;
    timer->checkFrameRate = checkFrameRate;
    timer->carsTiming = carTime;
    timer->carsTime = carTime;
    return timer;
}

void Timer(TIMER* timer, FROG* frog) {
    timer->checkFrameRate += timer->frameRate;
    if (timer->checkFrameRate >= timer->maxFrameRate) {             // Update time and avaliable moves during the game
        timer->time++;
        timer->checkFrameRate = 0;
        if (frog->remainingMoves < frog->maxMoves) {
            frog->remainingMoves++;
        }
    }

    timer->carsTime--;
    if (timer->carsTime <= 0) {
        timer->carsTime = timer->carsTiming;
    }
}

//------------------------------------------------
//---------------  STATS FUNCTIONS ---------------
//------------------------------------------------

void UpdateStats(WIN* w, FROG* frog, TIMER* timer) {
    Timer(timer, frog);         // Update time and avaliable moves


    werase(w->window);
    box(w->window, 0, 0);                                   // Show stats
    mvwprintw(w->window, 1, 1, "Time: %d", timer->time);
    mvwprintw(w->window, 2, 1, "Moves: %d", frog->remainingMoves);
    mvwprintw(w->window, 3, 1, "Points: %d", frog->points);
    mvwprintw(w->window, 5, 1, "Name: Maciej");
    mvwprintw(w->window, 6, 1, "Surname: Drywa");
    mvwprintw(w->window, 7, 1, "Index: 203556");
    wrefresh(w->window);
}


//------------------------------------------------
//-------------  MAIN DRAW FUNCTIONS  -------------
//------------------------------------------------

void DrawMap(WIN* win, int** positionType, int rows, int cols) {
    box(win->window, 0, 0);                   // Create the border

    for (int y = 1; y < rows - 1; y++) {
        for (int x = 1; x < cols - 1; x++) {
            switch (positionType[y][x]) {
            case GRASS:                                                 // Draw grass
                wattron(win->window, COLOR_PAIR(GRASS_COLOR));
                mvwaddch(win->window, y, x, ' ');
                wattroff(win->window, COLOR_PAIR(GRASS_COLOR));
                break;
            case ROAD:                                                                                            // Draw road  
                wattron(win->window, COLOR_PAIR(ROAD_COLOR));
                mvwaddch(win->window, y, x, ' ');
                wattroff(win->window, COLOR_PAIR(ROAD_COLOR));
                break;
            case DESTINATION:                                                                              // Draw destination
                wattron(win->window, COLOR_PAIR(DESTINATION_COLOR));
                mvwaddch(win->window, y, x, ' ');
                wattroff(win->window, COLOR_PAIR(DESTINATION_COLOR));
                break;
            case OBSTACLE:                                                                                // Draw obstacles
                wattron(win->window, COLOR_PAIR(OBSTACLE_COLOR));
                mvwaddch(win->window, y, x, OBSTACLE_SIGN);
                wattroff(win->window, COLOR_PAIR(OBSTACLE_COLOR));
                break;

            case COIN:                                                                            // Draw coins
                wattron(win->window, COLOR_PAIR(COIN_COLOR));
                mvwaddch(win->window, y, x, COIN_SIGN);
                wattroff(win->window, COLOR_PAIR(COIN_COLOR));
                break;
            }
        }
    }
    wrefresh(win->window);
}


void DrawGame(WIN* playwin, WIN* statwin, FROG* frog, TIMER* timer, int roadPositions[], int** positionType, int roadCount, int rows, int cols) {
    // Wyczyść okna
    wclear(playwin->window);
    wclear(statwin->window);


    // Rysowanie dróg i planszy
    DrawMap(playwin, positionType, rows, cols);     // Draw map

    UpdateStats(statwin, frog, timer);                    // Update stats

    // Rysowanie żaby
    DrawFrog(frog);              // Draw frog       

    wrefresh(playwin->window);
    wrefresh(statwin->window);
}

//------------------------------------------------
//------------  LOGIC OF THE GAME ----------------
//------------------------------------------------


bool CheckCollision(WIN* playwin, WIN* statwin, FROG* frog, STORK* stork, CAR cars[], TIMER* timer, int carsAndRoadsCount) {
    if (CarColision(frog, cars, timer, carsAndRoadsCount) || StorkColision(frog, stork, timer)) {       // Check colisions with cars and stork
        wclear(playwin->window);                                                                        // And print the game results
        wclear(statwin->window);
        mvwprintw(playwin->window, 1, 1, "You lose!");
        mvwprintw(playwin->window, 3, 1, "Points: %d", frog->points);           // Print the game result
        mvwprintw(playwin->window, 4, 1, "Time: %d", timer->time);
        wrefresh(playwin->window);
        wrefresh(statwin->window);
        napms(5000);
        return true;
    }
    return false;
}


void SaveHighScore(const char* filename, int score, int time) {
    FILE* file = fopen(filename, "w");
    if (file != NULL) {
        fprintf(file, "%d %d", score, time);        // Save high score
        fclose(file);
    }
}

void LoadHighScore(const char* filename, int* score, int* time) {
    FILE* file = fopen(filename, "r");
    if (file != NULL) {
        if (fscanf(file, "%d %d", score, time) != 2) {
            *score = 0;
            *time = 0;              // Load high score
        }
        fclose(file);
    }
    else {
        *score = 0;
        *time = 0;
    }
}

bool CheckWin(WIN* playwin, WIN* statwin, FROG* frog, TIMER* timer, int** positionType) {
    if (frog->y == 1) {                 // Check if frog reached the destination
        wclear(statwin->window);
        wclear(playwin->window);
        int highScore, bestTime;
        LoadHighScore("highscore.txt", &highScore, &bestTime);      // Load high score from file
        if (frog->points > highScore || (frog->points == highScore && timer->time < bestTime)) {
            SaveHighScore("highscore.txt", frog->points, timer->time);          // Save new high score
            mvwprintw(playwin->window, 2, 1, "New High Score!");
        }
        mvwprintw(playwin->window, 1, 1, "You win!");
        mvwprintw(playwin->window, 3, 1, "Points: %d", frog->points);          // Print the game results
        mvwprintw(playwin->window, 4, 1, "Time: %d", timer->time);
        wrefresh(playwin->window);
        wrefresh(statwin->window);
        napms(5000);
        return true;
    }
    return false;
}


//------------------------------------------------
//---------  GAME PARAMETERS FUNCTIONS -----------
//------------------------------------------------

void LoadConfig(const char* filename, int* rows, int* cols, int* carAndRoads, char* frogSign, char* carSign, int* carSpeed, int* carLength) {
    FILE* file = fopen(filename, "r");
    if (file != NULL) {
        if (fscanf(file, "ROWS: %d\n COLS: %d\n CAR_AND_ROADS: %d\n FROG_SIGN: %c\n CAR_SIGN: %c\n CAR_SPEED: %d\n CAR_LENGTH: %d",     // Load game parameters from file
            rows, cols, carAndRoads, frogSign, carSign, carSpeed, carLength) != 7) {
            *rows = 20;
            *cols = 30;
            *carAndRoads = 9;
            *frogSign = '@';
            *carSign = '#';
            *carSpeed = 3;                                  // If file is empty or parameters are incorrect,
            *carLength = 3;                                 // set default values
        }
    }
    else {
        *rows = 20;
        *cols = 30;
        *carAndRoads = 9;
        *frogSign = '@';
        *carSign = '#';
        *carSpeed = 3;
        *carLength = 3;
    }
}

void InitParameters(int roadPositions[], int** positionType, int rows, int cols, int roads) {

    for (int i = 0; i < rows; i++) {            // Set grass positions
        for (int j = 0; j < cols; j++) {
            positionType[i][j] = GRASS;
        }
    }

    for (int j = 0; j < cols; ++j) {                     // Set destination position    
        positionType[1][j] = DESTINATION;
    }


    bool* usedRows = new bool[rows];            // Used rows
    for (int i = 0; i < rows; i++) {
        usedRows[i] = false;
    }

    int roadsCreated = 0;
    while (roadsCreated < roads) {                  // Set road positions
        int roadY = rand() % (rows - 4) + 2;
        if (!usedRows[roadY]) {
            usedRows[roadY] = true;
            roadPositions[roadsCreated] = roadY;
            for (int j = 0; j < cols; ++j) {
                positionType[roadY][j] = ROAD;
            }
            roadsCreated++;
        }
    }
    delete[] usedRows;


    int obstaclesPlaced = 0;
    while (obstaclesPlaced < OBSTACLE_COUNT) {           // Set obstacles positions  
        int obstacleY = rand() % (rows - 2) + 1;
        int obstacleX = rand() % (cols - 2) + 1;
        if (positionType[obstacleY][obstacleX] == GRASS) {      // Obstacles must be on grass
            positionType[obstacleY][obstacleX] = OBSTACLE;
            obstaclesPlaced++;
        }
    }

    int coinsPlaced = 0;
    while (coinsPlaced < COINS_COUNT) {       // Set coins positions
        int coinY = rand() % (rows - 2) + 1;
        int coinX = rand() % (cols - 2) + 1;
        if (positionType[coinY][coinX] == GRASS && positionType[coinY][coinX] != OBSTACLE) {       // Coins must be on grass and 
            positionType[coinY][coinX] = COIN;                                                                                           // can't be on obstacles   
            coinsPlaced++;
        }
    }

}

void InitializeGame(WINDOW*& mainwin, WIN*& playwin, WIN*& statwin, FROG*& frog, STORK*& stork,
    TIMER*& timer, CAR*& cars, int**& positionType, int*& roadPositions, int& ROWS, int& COLS, int& MAX_CARS_ROADS,
    int& CAR_SPEED, int& CAR_LENGTH, char& FROG_SIGN, char& CAR_SIGN) {
    mainwin = Start();      // Setup main window         
    Welcome(mainwin);    // Welcome screen (menu)

    LoadConfig("config.txt", &ROWS, &COLS, &MAX_CARS_ROADS, &FROG_SIGN, &CAR_SIGN, &CAR_SPEED, &CAR_LENGTH);        // Loading game parameters from file

    cars = new CAR[MAX_CARS_ROADS];              // Init amount of cars

    positionType = new int* [ROWS];             // Init game map positions
    for (int i = 0; i < ROWS; i++) {
        positionType[i] = new int[COLS];
    }

    roadPositions = new int[MAX_CARS_ROADS];    // Init road positions

    InitParameters(roadPositions, positionType, ROWS, COLS, MAX_CARS_ROADS);        // Init game map (randomize positions on the map)

    playwin = Init(mainwin, ROWS, COLS, Y, X, MAIN_COLOR);                                      // Init subwindow for the game
    statwin = Init(mainwin, STATS_HEIGHT, STATS_WIDTH, Y, COLS + 1 + X, MAIN_COLOR);    // Init subwindow for the stats

    stork = InitStork(playwin, STORK_COLOR, STORK_SIGN, TIME_TO_STORK);                     // Init stork parameters
    frog = InitFrog(playwin, FROG_COLOR, FROG_SIGN, FROG_REMAINING_MOVES, positionType);    // Init frog parameters


    timer = InitTimer(statwin, START_TIME, FRAME_RATE, MAX_FRAME_RATE, CHECK_FRAME_RATE, CAR_SPEED * CAR_SPEED);    // Init timer parameters

    InitCars(playwin, cars, roadPositions, MAX_CARS_ROADS, CAR_LENGTH, CAR_SPEED, CARM_COLOR, CARS_COLOR, CAR_SIGN);    // Init cars parameters
    DrawGame(playwin, statwin, frog, timer, roadPositions, positionType, MAX_CARS_ROADS, ROWS, COLS);           // Draw map and game elements
}

void CleanupGame(WINDOW* mainwin, WIN* playwin, WIN* statwin, FROG* frog, STORK* stork, TIMER* timer, CAR* cars, int** positionType, int* roadPositions, int ROWS) {
    for (int i = 0; i < ROWS; i++) {
        delete[] positionType[i];
    }
    delete[] positionType;
    delete[] roadPositions;             // Cleanup all game parameters
    delete[] cars;
    delete frog;
    delete stork;
    delete timer;
    delwin(playwin->window);
    delwin(statwin->window);
    delwin(mainwin);
    endwin();
}

//------------------------------------------------
//----------------  MainLoop FUNCTION ------------
//------------------------------------------------

void MainLoop(WIN* statwin, WIN* playwin, FROG* frog, STORK* stork, int** positionType, CAR cars[], TIMER* timer, int carsAndRoadsCount,
    int carLenght, int carSpeed, int carMoveColor, int carStopColor, int carFrogColor) {
    keypad(playwin->window, TRUE);      // Can use arrows
    nodelay(playwin->window, TRUE);
    while (!CheckCollision(playwin, statwin, frog, stork, cars, timer, carsAndRoadsCount) &&
        !CheckWin(playwin, statwin, frog, timer, positionType)) {           // Check coliisions and win conditions

        UpdateStats(statwin, frog, timer);         // Update stats

        FrogMovement(playwin, frog, cars, carsAndRoadsCount, positionType, carStopColor, carFrogColor);  // Move frog

        MoveCars(cars, timer, frog, positionType, carsAndRoadsCount, carLenght, carSpeed, carMoveColor, carStopColor);      // Move cars

        MoveStork(stork, frog, timer, positionType);    // Move strok

        napms(timer->frameRate);                // Delay of the refresh rate

    }

    return;
}

//------------------------------------------------
//----------------  MAIN FUNCTION ----------------
//------------------------------------------------

int main() {
    srand(time(NULL));

    WINDOW* mainwin;
    WIN* playwin;
    WIN* statwin;
    FROG* frog;
    STORK* stork;
    TIMER* timer;
    CAR* cars;
    int** positionType;
    int* roadPositions;
    int ROWS, COLS, MAX_CARS_ROADS, CAR_SPEED, CAR_LENGTH;
    char FROG_SIGN, CAR_SIGN;

    while (true) {
        InitializeGame(mainwin, playwin, statwin, frog, stork, timer, cars, positionType,               // Init game parameters
            roadPositions, ROWS, COLS, MAX_CARS_ROADS, CAR_SPEED, CAR_LENGTH, FROG_SIGN, CAR_SIGN);

        MainLoop(statwin, playwin, frog, stork, positionType, cars, timer, MAX_CARS_ROADS, CAR_LENGTH,      // Main game loop
            CAR_SPEED, CARM_COLOR, CARS_COLOR, CARC_COLOR);

        CleanupGame(mainwin, playwin, statwin, frog, stork, timer, cars, positionType, roadPositions, ROWS);                             // Cleanup game parameters
    }

    return 0;
}