
// ntris, the online version
#include "Pentris.h"

int main(int argc, char** argv) {
    int port = DEFAULTPORT;
    char* address;
    int c;
    opterr = 0;

    onlineMode = OFFLINE;
    // check if we are a server, client, or an offline game
    while ((c = getopt(argc, argv, "sc:p:h")) != -1) {
        switch (c) {
            // ntris ran with -s, server mode
            case 's':
                onlineMode = SERVER;
                break;
            // ntris ran with -c:IP, client mode - enter IP now
            case 'c':
                onlineMode = CLIENT;
                address = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case '?':
                if (optopt == 'c') 
                    cout << "Usage: ./ntris -c<enter destination IP here>" << endl 
                            << "Using default port " << port << endl;
                else if (optopt == 'p')
                    cout << "Usage: ./ntris -s/c -p<enter port here>" << endl;
                else 
                    cout << "Unknown option " << (char)optopt << endl;
                exit(0);
                break;
        }
    }
    // using the argc/argv flags, initialize the online components
    initOnline(onlineMode, port, address);
  
    // run all SDL initialization routines
    initSDL();
    // get the block data
    openBlockData();
    // initialize and draw the board
    initBoards();

    // run the game loop
    gameState = PLAYING;
    gameLoop();

    // delete all the pointers
    cleanup();

    return 0;
}

void initOnline(int state, int port, char* address) {
    timeval t;
    vector<int>* pointer;
    vector<int> seed;

    switch (state) {
        case OFFLINE:
            gettimeofday(&t, NULL);
            seedPRG(t.tv_usec, true);
            break;
        case SERVER: 
            gettimeofday(&t, NULL);
            seedPRG(t.tv_usec);
            initServer(port);
            
            // send the random seed to the client so both players get the same blocks
            seed.push_back(t.tv_usec);
            sendMessage(vectorToString(&seed));
            nextSeed = prg();
            break; 
        case CLIENT: {
            initClient(port, address);
            
            string token = nextToken();
            while (token == "") {
                token = nextToken();
            }
            pointer = stringToVector(token);
            seed = *pointer;
            seedPRG(seed[0]);
            nextSeed = prg();
            delete pointer;
            break; }
    }
}

void initSDL() {
    SCREENWIDTH = SQUAREWIDTH*NUMCOLS + 11*SIDEBOARD;
    SCREENHEIGHT = SQUAREWIDTH*(NUMROWS - MAXBLOCKSIZE + 1);
    if (onlineMode != OFFLINE) 
        SCREENWIDTH = 2*SCREENWIDTH;
    
    // initialize SDL 
    SDL_Init(SDL_INIT_VIDEO);

    // set the title bar 
    SDL_WM_SetCaption("ntris", "ntris");

    // create window 
    screen = SDL_SetVideoMode(SCREENWIDTH, SCREENHEIGHT, BITDEPTH, 0);
    SDL_FillRectangle(screen, 0, 0, SCREENWIDTH, 0, SCREENHEIGHT, BLACK);
    
    // draw the hold rectangle in the GUI to the right - only needs to be done once
    SDL_FillRectangle(screen, SQUAREWIDTH*NUMCOLS+6, 38*(PREVIEW+2)-22, SQUAREWIDTH*NUMCOLS+44, 38*(PREVIEW+2)-22, 39, WHITE);
    SDL_FillRectangle(screen, SQUAREWIDTH*NUMCOLS+7, 38*(PREVIEW+2)-21, SQUAREWIDTH*NUMCOLS+43, 38*(PREVIEW+2)-21, 37, BLACK);

    // there is no key repeat - we only catch key events as keys are pressed or released
    SDL_EnableKeyRepeat(120, 30);
}

void openBlockData() {
    string line;
    int x, y, difficulty;
    
    ifstream myfile;
    if (onlineMode == OFFLINE)
        myfile.open("blockData.dat");
    else
        myfile.open("multiplayerData.dat");

    if (myfile.is_open()) {
        // the file begins with the number of different types of blocks at each difficulty level
        getline (myfile, line);
        difficultyLevels = atoi(line.c_str());
        for (int i = 0; i < difficultyLevels; i++) {
            getline (myfile, line);
            numBlockTypes[i] = atoi(line.c_str());
        }
        difficulty = 0;
        getline (myfile, line);
        numSymbols = atoi(line.c_str());

        // blank line before the block data
        getline(myfile, line);
        
        for (int i = 0; i < numBlockTypes[difficultyLevels-1] + numSymbols; i++) {
            blockData[i] = new Block();
            // the first two lines of this block's data record the starting position
            getline (myfile, line);
            // first, how far off center the x-coordinate is
            blockData[i]->x = NUMCOLS/2 + atoi(line.c_str());
            // then the y-coordinate
            getline (myfile, line);
            blockData[i]->y = atoi(line.c_str());
            // next, the number of squares in the block
            getline (myfile, line);
            blockData[i]->numSquares = atoi(line.c_str());
            // read each square's local coordinates from memory
            for (int j = 0; j < blockData[i]->numSquares; j++) {
                getline (myfile, line);
                x = atoi(line.c_str());
                getline (myfile, line);
                y = atoi(line.c_str());
                // read x and y, record in blockData
                blockData[i]->squares[j] = Point(x, y);
            }
            // lastly, read the color
            getline (myfile, line);
            unsigned int color = colorCode(atoi(line.c_str()));
            blockData[i]->color = color;
            if (onlineMode != OFFLINE) blockData[i]->color = difficultyColor(color, (1.0f*difficulty)/(difficultyLevels-1));
            blockData[i]->color = mixedColor(BLACK, blockData[i]->color, 0.2);

            // record the block's height in blockData
            blockData[i]->height = calculateBlockHeight(blockData[i]);

            // blank line after each block's data
            getline (myfile, line);

            if ((difficulty < difficultyLevels) and (i > numBlockTypes[difficulty])) difficulty++;
            if (i >= numBlockTypes[difficultyLevels-1]) 
                blockData[i]->color = difficultyColor(STAIRCASE, (1.0f*(i-numBlockTypes[difficultyLevels-1]+1))/(difficultyLevels-1));
        }
        
        // get data about any blocks that do not rotate
        getline (myfile, line);
        x = atoi(line.c_str());
        // there are x blocks that do not rotate - read their indices now
        for (int i = 0; i < x; i++) {
            getline (myfile, line);
            blockData[atoi(line.c_str())]->rotates = false;
        }

        myfile.close();
    } else cout << "Unable to open file" << endl;
}

int calculateBlockHeight(Block* block) {
    int highest = 0;
    int lowest = 0;

    for (int i = 0; i < block->numSquares; i++) {
        if (block->squares[i].y < lowest)
            lowest = block->squares[i].y;
        if (block->squares[i].y > highest)
            highest = block->squares[i].y;
    }
    return highest - lowest + 1;
}

void initBoards() {
    int numBlocks = numBlockTypes[difficultyLevels-1];

    // the main board appears in the top-left corner
    if (onlineMode == OFFLINE) {
        mainBoard = new Board(0, 0, SQUAREWIDTH, SIDEBOARD, numBlocks, blockData);
        mainBoard->loadSprites(screen, SCREENWIDTH, SCREENHEIGHT);
    } else {
        mainBoard = new Board(0, 0, SQUAREWIDTH, SIDEBOARD, numBlocks, blockData, MULTIPLAYER);
        mainBoard->loadSprites(screen, SCREENWIDTH, SCREENHEIGHT);
        advBoard = new Board(9*SCREENWIDTH/16, SCREENHEIGHT/8, 3*SQUAREWIDTH/4, 3*SIDEBOARD/4, numBlocks, blockData, MULTIPLAYER);
    }
    playTetrisGod(mainBoard);
    mainBoard->draw(screen);
    if (onlineMode != OFFLINE)
        advBoard->draw(screen);
}

void gameLoop() {
    int curTime, lastTime, lastSecond, numFrames;
    timeval t;
    
    // numFrames stores the number of frames drawn this second
    numFrames = 0;
    gettimeofday(&t, NULL);
    curTime = t.tv_sec*TICKS_PER_SEC + t.tv_usec;
    lastTime = curTime;
    lastSecond = curTime;
    // keep track of frameCount for periodic updates
    frameCount = 0;

    while (gameState >= PLAYING) {
        // check current time - if one frameDelay has passed draw the next frame 
        gettimeofday(&t, NULL);
        curTime = t.tv_sec*TICKS_PER_SEC + t.tv_usec;
        if (curTime > lastTime + FRAMEDELAY) {            
            lastTime = curTime;

            if (curTime > lastSecond + TICKS_PER_SEC) {
                //cout << "FPS = " << 1.0f*numFrames*TICKS_PER_SEC/(curTime - lastSecond) << endl;                                
                lastSecond = curTime;
                numFrames = 1;
            } else {
                numFrames++;
            }
   
            // update frameCount, mod MAXFRAMECOUNT
            frameCount = (frameCount+1)%MAXFRAMECOUNT;

            SDL_Event event;
            // look for a keyboard event
            if (SDL_PollEvent(&event)) {
                HandleEvent(event, mainBoard);
            }
            mainBoard->timeStep(frameCount);
            playTetrisGod(mainBoard);
        
            // draw the board and update the visible buffer
            mainBoard->draw(screen);
            if (onlineMode != OFFLINE) {
                updateAdvBoard();
                advBoard->draw(screen);
            }
            SDL_UpdateRect(screen, 0, 0, 0, 0);
     
        }
    }
}

bool isHoldKey(SDLKey key, int* hold) {
    switch (key) {
        case SDLK_q:
            *hold = 0;
            return true;
        case SDLK_w:
            *hold = 1;
            return true;
        case SDLK_e:
            *hold = 2;
            return true;
        case SDLK_r:
            *hold = 3;
            return true;
        case SDLK_t:
            *hold = 4;
            return true;
        case SDLK_y:
            *hold = 5;
            return true;
        case SDLK_u:
            *hold = 6;
            return true;
        case SDLK_i:
            *hold = 7;
            return true;
        case SDLK_o:
            *hold = 8;
            return true;
        case SDLK_p:
            *hold = 9;
            return true;
        case SDLK_a:
            *hold = 10;
            return true;
        case SDLK_s:
            *hold = 11;
            return true;
        case SDLK_d:
            *hold = 12;
            return true;
        case SDLK_f:
            *hold = 13;
            return true;
        case SDLK_g:
            *hold = 14;
            return true;
        case SDLK_h:
            *hold = 15;
            return true;
        case SDLK_j:
            *hold = 16;
            return true;
        case SDLK_k:
            *hold = 17;
            return true;
        case SDLK_l:
            *hold = 18;
            return true;
        case SDLK_z:
            *hold = 19;
            return true;
        case SDLK_x:
            *hold = 20;
            return true;
        case SDLK_c:
            *hold = 21;
            return true;
        case SDLK_v:
            *hold = 22;
            return true;
        case SDLK_b:
            *hold = 23;
            return true;
        case SDLK_n:
            *hold = 24;
            return true;
        case SDLK_m:
            *hold = 25;
            return true;
        case SDLK_LSHIFT:
            *hold = 26;
            return true;
    }
    return false;
}

void HandleEvent(SDL_Event event, Board* board) {
    int key, hold;

    switch (event.key.keysym.sym) {
        // translate the key pressed into one of the key codes
        case SDLK_ESCAPE:
            key = ESCAPE;
            gameState = QUIT;
            break;
        case SDLK_UP:
            key = MOVEUP;
            break;
        case SDLK_DOWN:
            key = MOVEDOWN;
            break;
        case SDLK_LEFT:
            key = MOVELEFT;
            break;
        case SDLK_RIGHT:
            key = MOVERIGHT;
            break;
        case SDLK_SPACE:
            key = MOVEDROP;
            break;
        case SDLK_RETURN:
            key = ENTER;
            break;
        default:
            if (isHoldKey(event.key.keysym.sym, &hold)) {
                key = hold + MOVEHOLD;
            } else {
                return;
            }
    }    
    
    switch (event.type) {
        // run the appropriate board key input event, passing this key
        case SDL_QUIT:
            board->keyPress(ESCAPE);
            gameState = QUIT;
            break;
        case SDL_KEYDOWN:
            board->keyPress(key);
            break;
        case SDL_KEYUP:
            board->keyRelease(key);
            break;
    }
}

void playTetrisGod(Board* board) {    
    int attack, type;
    int numNeeded = board->numBlocksNeeded();
    int score = board->getScore();

    for (int i = 0; i < numNeeded; i++) {
        if (onlineMode == OFFLINE) {
            type = prg()%numBlockTypes[difficultyLevel(score)];
        } else {
            attack = min(board->getNextAttack(), difficultyLevels-1);
            if (attack == 0)
                type = prg()%(numBlockTypes[0]);
            else
                type = prg()%(numBlockTypes[attack]-numBlockTypes[attack-1]) + numBlockTypes[attack-1];
        }
        board->queueBlock(type);
    }
}

// takes in a score s and returns the current difficulty level
int difficultyLevel(int s) {
    float x, p, r;

    if (difficultyLevels == 1)
        return 0;
    // get a random p uniformly from [0, 1]
    p = 1.0f*(prg()%(int(power(2, 12))))/power(2,12);
    // calculate the current ratio r between the probability of different difficulties
    x = 2.0*(s - HALFRSCORE)/HALFRSCORE;
    r = (MAXR-MINR)*(x/sqrt(1+x*x) + 1)/2 + MINR;
    // run through difficulty levels
    for (int i = 1; i < difficultyLevels; i++) {
        x = 2.0f*(s - (SCOREINTERVAL*i))/SCOREINTERVAL;
        // compare p to a sigmoid which increases to 1 when score passes SCOREINTERVAL*i
        if (p > power(r, i)*(x/sqrt(1+x*x) + 1)/2)
            // if p is still above this sigmoid, we are not yet at this difficulty level
            return i-1;
    }
    return difficultyLevels - 1;
}

// raise a float a to a power b using log b depth iteration
float power(float a, int b) {
    if (b == 0) {
        // base case, return 1
        return 1;
    } else if (b == 1) {
        // base case, return a
        return a;
    } else {
        // calculate inductive step
        float x = power(a, b/2);
        if (b%2 == 0) {
            // square
            return x*x;
        } else {
            // multiply by a if necessary
            return a*x*x;
        }
    }
}

void updateAdvBoard() {
    string token;
    vector<int>* events;
    vector<int>* crossEvents;

    events = mainBoard->getEvents();
    if (events != NULL) {
        sendMessage(vectorToString(events));
        delete events; 
    }
    
    while ((token=nextToken()) != "") {
        events = stringToVector(token);
        if (events->size() > 0) {
            crossEvents = advBoard->doEvents(events);
            if (crossEvents != NULL) {
                mainBoard->doCrossEvents(crossEvents);
                delete crossEvents;
            }
        }
        delete events;
    }

    if (mainBoard->getBoardState() == COUNTDOWN) {
        seedPRG(nextSeed);
        nextSeed = prg();
    }
}

void cleanup() {
    // kill SDL and close online connections
    SDL_Quit();
    if (onlineMode != OFFLINE)
        closeConnections();
    // delete all pointers
    for (int i = 0; i < numBlockTypes[difficultyLevels-1]; i++) {
        delete blockData[i];
    }
}

