
using namespace std;

#include "SDL.h"
extern "C" {
#include "SDL_prims.h"
}
#include "Color.h"
#include "Sprite.cpp"
#include <iostream>
#include <list>
#include <vector>

// using up to dekaminoes right now
#define MAXBLOCKSIZE 10
// size of the board - number of rows and columns
// the first MAXBLOCKSIZE-1 rows are secret rows, above the top of the screen
#define NUMROWS (24+MAXBLOCKSIZE-1)
#define NUMCOLS 12

// board states
#define RESET 0
#define PLAYING 1
#define PAUSED 2
#define GAMEOVER 4
#define COUNTDOWN 6
#define QUIT -1

// the amount of time each "second" takes during a countdown
#define SECOND 54
#define NUMSECONDS 3

// game modes
#define SINGLEPLAYER 0
#define MULTIPLAYER 1

// event types
#define PLACEBLOCK 0
#define GETNEXTBLOCK 1
#define QUEUEBLOCK 2
#define SENDATTACK 3
#define RECEIVEATTACK 4
#define VICTORY 5
#define RESETBOARD 6
#define MAXEVENTS 32

// illegal block flags - these record why a block is illegal
// given in order of priority, so first a block is tested against
// the edges of the board, then it is tested for overlap
#define OK 0
#define TOPEDGE 1
#define RIGHTEDGE 2
#define BOTTOMEDGE 3
#define LEFTEDGE 4
#define OVERLAP 5

// this variable records how many frames go by before gravity is applied
#define GRAVITY 1000000000
// the maximum number of times a block can be shoved away from an obstacle
#define MAXSHOVEAWAYS 2
// the number of frames before a block sticks in one place
#define MAXLOCALSTICKFRAMES 24
// the number of frames before a block sticks anywhere
#define MAXGLOBALSTICKFRAMES 120 

// constants holding movement direction data
#define MOVEUP 1
#define MOVERIGHT 2
#define MOVEDOWN 3
#define MOVELEFT 4
#define MOVEDROP 5
#define ENTER 6
#define PAUSE 7
#define ESCAPE 8
// Constants 9-34 are hold constants - record the value of the first and the
// number of holds
#define MOVEHOLD 9
#define NUMHOLDS 27
const int holdToQwerty[NUMHOLDS] = {16, 22, 4, 17, 19, 24, 20, 8, 14, 15,
                                    0, 18, 3, 5, 6, 7, 9, 10, 11,
                                    25, 23, 2, 21, 1, 13, 12, -1};

// the number of blocks we preview
#define PREVIEW 5
// the number of frames used to animate the preview list
#define PREVIEWANIMFRAMES 3

// constants about the animated attack symbols
#define ANIMSIZE 3
#define MAXANIMATIONS 4
#define ANIMFRAMES 72
// the size of the numbers in the attack queue
#define ATTACKSIZE (squareWidth/5) 

// point struct - contains an x and a y
struct Point {
    int x;
    int y;
    // default constructor - run when an array of points is initialized
    Point() { }
    // construct a point with a given x and y
    Point(int initX, int initY) {
        x = initX;
        y = initY;
    }
};

// struct which contains the data for the current block
struct Block {
    // position of the anchor point on the board
    int x;
    int y;
    // the number of 90-degree clockwise turns to transform the block by
    int angle;
    // the number of squares in this nomino - the n, which is at most 5
    int numSquares;
    // the displacement of the other squares from the anchor, in standard position
    Point squares[MAXBLOCKSIZE];
    // by default, blocks are RED
    unsigned int color;
    // records the number of shoveaways this block has used - only up to MAXSHOVEAWAYS
    int shoveaways;
    // the number of frames the block has left before it sticks at the current position
    int localStickFrames;
    // the number of frames the block has left before it sticks anywhere
    int globalStickFrames;
    // the number of squares high the block is
    int height;
    // default constructor
    Block() {
        x = 0;
        y = 0;
        angle = 0;
        numSquares = 0;
        color = RED;
        shoveaways = 0;
        localStickFrames = MAXLOCALSTICKFRAMES;
        globalStickFrames = MAXGLOBALSTICKFRAMES;
        rotates = true;
        height = 0;
    }
    // can the block rotate? singletons and 2x2s cannot
    bool rotates;
    // the distance between this block and its shadow - how far the block
    // will fall if it is hard-dropped
    int rowsDropped;
};

// stores the board data
class Board {
    public:
        // default ctor() and dtor()
        Board(); ~Board();
        // ctor with info - pass it the x and y position, the squarewidth, 
        // the sideboard width, and a pointer to the blockData array
        Board(int, int, int, int, int, Block**, int=SINGLEPLAYER);
        // pass this board pointers to the sprites it draws with
        void loadSprites(SDL_Surface*, int, int);

        // relay keyboard input to the board through these methods
        void keyPress(int);
        void keyRelease(int);

        // getters for private variables
        int getScore();
        int getBoardState();
        vector<int>* getEvents();
        int getNextAttack();
        
        // the Tetris client will determine the distribution of blocks
        // and play "Tetris god", queue up blocks in the preview list
        int numBlocksNeeded();
        void queueBlock(int);
        // dow a sequence of events or register an attack from a vector
        vector<int>* doEvents(vector<int>*);
        void doCrossEvents(vector<int>*);

        // moves a block according to user input and Tetris game logic
        void timeStep(int);
        // draw the board on the screen surface
        void draw(SDL_Surface*);    
    private:
        // position
        int xPos, yPos;
        // display constants
        int squareWidth;
        int sideBoard, boardWidth, boardHeight;
        // a pointer to the array containing information about every block
        int numBlocks;
        Block** blockData;
    
        // in a multiplayer game, record the events and attacks taking place on this board
        vector<int> events;
        list<int> attacks;

        // current score
        int score, combo;
        // when the board state is negative, exit
        int boardState, gameMode;
        // holds the player's movement input
        list<int>* moveDir;
        // keyboard actions which have already been taken - do not repeat
        bool rotated;
        bool dropped;
        bool held[NUMHOLDS];
        bool entered;

        // stores the information for the current block in player
        Block* curBlock;
        // types of the blocks in play and in hold
        int curBlockType;
        int heldBlockType[NUMHOLDS];
        // the blocks ahead, stored in a list - pushed right, popped left
        list<int> preview;
        // how many frames we are into the preview animation, and the displacement from the old block
        int previewAnim;
        int previewOffset;

        // these booleans tell us how much to erase
        // true when the hold is used - redraw the hold
        bool holdUsed[NUMHOLDS];
        // if the board changes we redraw the entire thing
        bool boardChanged;
        // if a block was just held we use it to erase, not the current block
        int lastHoldUsed;
        // array which contains the type of block in each square
        int board[NUMCOLS][NUMROWS];
        // array which stores the number of blocks in each row
        int blocksInRow[NUMROWS];
        // records the highest non-empty row - all rows with index less than
        // this number are empty - for faster drawing
        int highestRow;
        // stores the block's last position, for quick erasing
        Block* oldBlock;
        
        // pointers to the text sprites, and a flag which is true if we need to draw them
        Sprite* font;
        Sprite* numbers;
        Sprite* paused;
        Sprite* gameover;    
        Block* animBlock[MAXANIMATIONS];
        Sprite* animation[MAXANIMATIONS];
        bool drawSprites;

        // reset the board - ready for another game
        void resetBoard();

        // a list helper method: add a movement direction to the list 
        static void addItemToList(list<int>*, const int, const int=1);

        // on a rotation, attempts to shove a block upward away from an obstruction
        bool shoveaway(Block*);
        // places a block on the board
        void placeBlock(Block*);
        // gets the next block for the player
        void getNextBlock(int=-1);
        // checks whether a block is in legal position
        int checkBlock(Block*);
        // calculates how many squares this block can fall if dropped
        int calculateRowsDropped(Block*);
        // removes any full rows from the board, returns the number removed
        int removeRows();

        // draws just the section of the board that changed
        void drawBoard(SDL_Surface* sfc);
        // redraws the entire board, square by square 
        void redrawBoard(SDL_Surface* sfc, bool=false, unsigned int=RED);
        // draws the GUI to the right of the board
        void drawGUI(SDL_Surface* sfc, bool=false, unsigned int=RED);
        // draws the hold block on the right
        void drawHold(SDL_Surface* sfc, int hold, bool=false, bool=false, unsigned int=RED);
        // draws a Tetris block
        // the first bool flag asks if we are drawing the block or erasing it 
        // the second is set to true if we want to draw the block's shadow 
        void drawBlock(SDL_Surface* sfc, Block*, bool=false, bool=false, int=0, int=0, int=0);
        // draws a small block for the right-hand side GUI
        void drawSmallBlock(SDL_Surface* sfc, Block*, int, int, int, float=0.0f, bool=false, unsigned int=RED);
        // draws the correct color into a particular square of the board
        void drawSquare(SDL_Surface* sfc, int, int, int, bool=false);

        // animation routines
        void startAnimation(Block*, int);
        void startAnimation(int, int, int);
        void eraseAnimations(SDL_Surface*);
        void drawAnimations(SDL_Surface*, bool=false, unsigned int=RED);
};

