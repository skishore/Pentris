
using namespace std;

#include "Board.cpp"
#include "Online.cpp"
#include "PRG.h"
#include <cstdlib>
#include <cstdio>
#include "SDL.h"
#include <unistd.h>
#include "sys/time.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <cstring>
#include <complex>
#include <math.h>

// display constants
#define SQUAREWIDTH 20
#define SIDEBOARD 7*SQUAREWIDTH/2
int SCREENWIDTH;
int SCREENHEIGHT;

// frameCount is a rhythm variable - used to keep periodic updates, like gravity, on track
// on each frame, frameCount is increased by 1
int frameCount;
// every MAXFRAMECOUNT frames, frameCount resets - the value is 840 because we can time
// actions with a period of up to 8 frames - 840 = LCM(1,2,3,4,5,6,7,8)
#define MAXFRAMECOUNT 840

// game loop constants
#define FRAMERATE 60
#define TICKS_PER_SEC 1000000
#define FRAMEDELAY (TICKS_PER_SEC/FRAMERATE)

// online modes
#define OFFLINE 0
#define SERVER 1
#define CLIENT 2
#define FAILED 3
int onlineMode = 0;

// when curTime > nextTime, draw the next frame 
int nextTime;
// SDL surface which holds the screen in memory
SDL_Surface* screen;

// the score at which we transitition between difficulty levels
#define SCOREINTERVAL 60 
// the minimum value of r, the probability of getting the next difficulty level
#define MINR 0.1
// the maximum value of r
#define MAXR 0.9
// the score at which r, the probability of getting to the next
// difficulty level, is halfway between MINR and 1
#define HALFRSCORE 480
// the number of difficulty levels
int difficultyLevels;
// the number of different kinds of blocks at each difficulty level, and
// the number of special "symbol" blocks
int numBlockTypes[7];
int numSymbols;
// stores the information about all the different kinds of blocks
Block* blockData[13000];

int gameState;
// the boards which conduct all the Tetris logic and handles drawing
Board* mainBoard;
Board* advBoard;
// the next seed we will use for the PRG - the first string outputted by the last seed
unsigned int nextSeed; 

// first procedure called
int main(int, char**);
// using the argc/argv flags, initialize the online components
void initOnline(int, int, char*);
// runs SDL initialization routines and initializes the sprites
void initSDL();
// loads the block data from file
void openBlockData();
// calculates the height, in squares of a given block
int calculateBlockHeight(Block*);
// initializes both the main board and (in an online match) the opponent's
void initBoards();
// runs the game loop
void gameLoop();
// handles keyboard input
void HandleEvent(SDL_Event, Board*);
// queues up new blocks on the board, as needed, with the right distribution
void playTetrisGod(Board*);
// returns the current difficulty level as a function of the score
int difficultyLevel(int);
// random function to take integers to powers
float power(float, int);
// update our image of the adversary board from the web data
void updateAdvBoard();
// cleanup memory
void cleanup();

