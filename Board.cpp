
using namespace std;

#include "Board.h"

Board::Board() {}

Board::Board(int x, int y, int square, int side, int n, Block** data, int mode) {
    xPos = x; yPos = y;
    squareWidth = square;
    sideBoard = side;
    boardWidth = squareWidth*NUMCOLS + sideBoard;
    boardHeight = squareWidth*(NUMROWS-MAXBLOCKSIZE+1);
    numBlocks = n;
    blockData = data;
    drawSprites = false;
    
    gameMode = mode;
    if (gameMode == MULTIPLAYER)
        events.clear();

    moveDir = new list<int>();

    resetBoard();
}

void Board::loadSprites(SDL_Surface* sfc, int width, int height) {
    if (gameMode == SINGLEPLAYER) {
        // in the one-player game, we use the numbers, gameover, and paused sprites   
        numbers = new Sprite();
        numbers->loadSprite(8, 10, BITDEPTH, "numbers.bmp", 10, 2, sfc, width, height);
        gameover = new Sprite();
        gameover->loadSprite(192, 32, BITDEPTH, "gameover.bmp", 1, 1, sfc, width, height);
        // position gameover in the center for easy drawing
        gameover->x = (width-192)/2;
        gameover->y = (height-32)/2;
        paused = new Sprite();
        paused->loadSprite(94, 18, BITDEPTH, "paused.bmp", 1, 1, sfc, width, height);
        paused->x = (width-94)/2;
        paused->y = (height-18)/2;
    } else {
        // load and position the won/lost countdown message
        gameover = new Sprite();
        gameover->loadSprite(192, 32, BITDEPTH, "wonlost.bmp", 2, 3, sfc, width, height);
        gameover->x = (width/2-192)/2;
        gameover->y = (height-32)/2;
        /* // load the animation sprites 
        for (int i = 0; i < MAXANIMATIONS; i++) {
            animBlock[i] = new Block();
            animation[i] = new Sprite();
            animation[i]->loadSprite(5*ANIMSIZE+6, 5*ANIMSIZE+6, BITDEPTH, "animation.bmp", 2, 1, sfc, width, height);
        } */
    } 
        
    drawSprites = true;
}


void Board::resetBoard() {
    // log this event in multiplayer games
    if (gameMode == MULTIPLAYER) events.push_back(RESETBOARD);

    // no commands have been given yet
    rotated = false;
    dropped = false;
    held = false;
    entered = false;

    // still have a hold for this block
    holdUsed = false;

    // to start off, the board is entirely BLACK
    for (int y = 0; y < NUMROWS; y++) {
        for (int x = 0; x < NUMCOLS; x++) {
            board[x][y] = BLACK;
        }
        blocksInRow[y] = 0;
    }
    // in the begininning, no rows contain any squares
    highestRow = NUMROWS;
    // the board has changed - redraw
    boardChanged = true;
    // no block was just held - we don't have to erase specially
    justHeld = false;
    // start the preview without animation
    preview.clear();
    previewAnim = 0;
    previewOffset = 0;
    // no block is held - type is -1, the null type
    heldBlockType = -1;
    curBlockType = -1; 
    oldBlock = new Block();
    curBlock = NULL;

    /*if ((gameMode == MULTIPLAYER) and (drawSprites)) {
        for (int i = 0; i < MAXANIMATIONS; i++) {
            animBlock[i]->shoveaways = 0;
        }
    }*/

    // reset the score and start the game
    score = 0; combo = 0;
    attacks.clear();
    boardState = RESET;
}

// adds item x to list l, unless x already appears in l maxTimes times 
// by default, maxTimes is 1, so there is only one entry in the list for each direction
void Board::addItemToList(list<int>* l, const int x, const int maxTimes) {
    list<int>::iterator i;    
    int timesFound = 0;

    // add a MOVEDIR to the list of directions currently held down
       // The order of these directions is important - the first one
    // determines which way the player faces
    for (i = l->begin(); i != l->end(); i++)
        if (*i == x)
            timesFound++;
    if (timesFound < maxTimes)                    
        l->push_back(x);
}

void Board::keyPress(int key) {
    if (key == ESCAPE) {
        // exit ntris
        boardState = QUIT;
    } else if ((key == ENTER) or (key == PAUSE)) {
        if (entered == false) {
            // either pause or enter was pressed - multiple effects
            if (boardState >= GAMEOVER) {
                if ((key == ENTER) and (gameMode == SINGLEPLAYER))
                    resetBoard();
            } else if (boardState >= PAUSED) {
                boardState = PLAYING;
                boardChanged = true;
            } else if (boardState == PLAYING) {
                if (gameMode == SINGLEPLAYER)
                    boardState = PAUSED;
                entered = true;
            }
        }
    } else {
        // a movement key was pressed - add it to the list of directions
        addItemToList(moveDir, key);
    }
}

void Board::keyRelease(int key) {
    if (key == ESCAPE) {
        // exit ntris
        boardState = QUIT;
    } else if ((key == ENTER) or (key == PAUSE)) {
        entered = false;
    } else {
        // a movement key was released, so remove it from moveDir
        moveDir->remove(key);
        // set the key's flag to false so we can rotate / drop / hold again        
        if (key == MOVEUP) 
            rotated = false;
        else if (key == MOVEDROP) 
            dropped = false;
        else if (key == MOVEHOLD) 
            held = false;
    }
}

int Board::getScore() { return score; }

int Board::getBoardState() { return boardState; }

vector<int>* Board::getEvents() {
    if (events.size() > 0) {
        vector<int>* v = new vector<int>(events);
        events.clear();
        return v;
    } else return NULL;
}

int Board::getNextAttack() {
    if (attacks.size() > 0) {
        int attack = attacks.front();
        attacks.pop_front();
        return attack;
    } else return 0;
}

int Board::numBlocksNeeded() { 
    if (curBlockType == -1)
        return PREVIEW - preview.size() + 1;
    return PREVIEW - preview.size();
}

void Board::queueBlock(int blockType) {
    // log this event in multiplayer games
    if (gameMode == MULTIPLAYER) {
        events.push_back(QUEUEBLOCK);
        events.push_back(blockType);
    }

    preview.push_back(blockType);
    if (boardState == RESET) boardState = PLAYING;
}

vector<int>* Board::doEvents(vector<int>* newEvents) {
    vector<int>* crossEvents = NULL;
    int i = 0;
    int size = newEvents->size();
    if (size > MAXEVENTS) size = MAXEVENTS;
    events = *newEvents;

    while (i < size) {
        if (events[i] == PLACEBLOCK) {
            curBlock->x = events[i+1];
            curBlock->y = events[i+2];
            curBlock->angle = events[i+3];
            
            placeBlock(curBlock);
            delete curBlock;
            curBlock = NULL;
            i = i + 4;
        } else if (events[i] == GETNEXTBLOCK) {
            getNextBlock((events[i+1] == 1));
            if (boardState == GAMEOVER) {
                if (crossEvents == NULL) crossEvents = new vector<int>();
                crossEvents->push_back(VICTORY);
            }
            previewAnim = 1;
            i = i + 2;
        } else if (events[i] == QUEUEBLOCK) {
            queueBlock(events[i+1]);
            if (attacks.size() > 0) attacks.pop_front();
            i = i + 2;
        } else if (events[i] == SENDATTACK) {
            if (crossEvents == NULL) crossEvents = new vector<int>();
            crossEvents->push_back(RECEIVEATTACK);
            crossEvents->push_back(events[i+1]);
            i = i + 2;
        } else if (events[i] == RECEIVEATTACK) {
            attacks.push_back(events[i+1]);
            i = i + 2;
        } else if (events[i] == RESETBOARD) {
            resetBoard();
            i = i + 1;
        }
    }

    events.clear();
    // return true if this board attacked the other
    return crossEvents;
}

void Board::doCrossEvents(vector<int>* crossEvents) {
    int i = 0;
    int size = crossEvents->size();
    if (size > MAXEVENTS) size = MAXEVENTS;

    while (i < size) {
        if ((*crossEvents)[i] == RECEIVEATTACK) {
            attacks.push_back((*crossEvents)[i+1]);
            // log the attack so the other side can see it too
            events.push_back(RECEIVEATTACK);
            events.push_back((*crossEvents)[i+1]);
            i = i + 2;
        } else if ((*crossEvents)[i] == VICTORY) {
            boardState = COUNTDOWN;
            gameover->frameCol = 2;
            i = i + 1;
        }
    }
}

/** Tetris game logic starts here!                          *
 *  The only public method is timestep(), which runs        *
 *  one step of Tetris logic. This method calls             * 
 *  checkBlock(), shoveaway(), and a host of others        **/

void Board::timeStep(int frame) {
    list<int>::iterator i;
    Point trans = Point(0, 0);
    int deltaAngle = 0;
    bool moved = false;

    if (boardState != PLAYING)
        return;

    // don't move a non-existent block
    if (curBlock == NULL) {
        getNextBlock();
        return;
    }

    for (i = moveDir->begin(); i != moveDir->end(); i++) {
        // record the current movement commands
        if ((*i == MOVERIGHT) or (*i == MOVELEFT)) {
            // the last command is the only one that counts 
            trans.x = MOVEDOWN - *i;
        } else if (*i == MOVEUP) {
            // the rotate command is stored in angle
            // rotate only happens once each time MOVEUP is pressed
            // afterwards we set the rotated flag to true
            if (rotated == false)   { 
                if (curBlock->rotates == true) {
                    deltaAngle++;
                } else {
                    moved = true;
                }
                rotated = true;
            }
        } else if (*i == MOVEDOWN) {
            // MOVEDOWN forces the block to drop
            trans.y++;
        } else if (*i == MOVEDROP) {
            if (dropped == false) {
                curBlock->y += curBlock->rowsDropped;
                placeBlock(curBlock);
                delete curBlock;
                curBlock = NULL;
                dropped =  true;
                return;
            }
        } else if (*i == MOVEHOLD) {
            if ((held == false) and (holdUsed == false)) {
                // get the next block by swapping
                delete curBlock;
                curBlock = NULL;
                getNextBlock(true);
                held = true;
                return;
            }
        }        
    }
    // clear keys to accept new input
    moveDir->clear();

    // account for gravity's action on the block
    if (frame%GRAVITY == 0) 
        trans.y = 1;

    if (trans.x != 0) {
        // try to move the block right or left, if it is legal
        curBlock->x += trans.x;
        if (checkBlock(curBlock) != OK) {
            // the left and right movement is obstructed - move back
            curBlock->x -= trans.x;
        } else {
            // record the fact that this block moved
            moved = true;
        }
    }
    
    if (deltaAngle != 0) {
        // try to rotate, if needed
        curBlock->angle += deltaAngle;
        // move left or right to make room to rotate
        // trans.x will record how far we move
        trans.x = 0;
        while ((checkBlock(curBlock)%OVERLAP == LEFTEDGE) or (checkBlock(curBlock)%OVERLAP == RIGHTEDGE)) {
            if (checkBlock(curBlock)%OVERLAP == LEFTEDGE) {
                // rotated off the left edge - move right to compensate
                curBlock->x++;
                trans.x++;
            } else {
                // same on the right edge
                curBlock->x--;
                trans.x--;
            }
        }
        // now the block has been rotated away from the edge
        int check = checkBlock(curBlock);
        if ((check != OK) and (check%OVERLAP != TOPEDGE)) {
            // try to shoveaway from the obstruction, if we have shoveaways left
            if ((curBlock->shoveaways >= MAXSHOVEAWAYS) or (shoveaway(curBlock) == false)) {
                // if there are still any overlaps, rotate back, and move back
                curBlock->angle -= deltaAngle;
                // also, revert to the original x position
                curBlock->x -= trans.x;
            } else {
                // we've burned a shoveaway on this block
                curBlock->shoveaways++;
                // record the fact that this block moved
                moved = true;
            }
        } else if (check%OVERLAP == TOPEDGE) {
            // above the screen - try to move down after rotation
            int deltaY = 1;
            curBlock->y++;
            while (checkBlock(curBlock)%OVERLAP == TOPEDGE) {
                deltaY++;
                curBlock->y++;
            }
            // now check if the block is in a free position
            if (checkBlock(curBlock) ==  OK) {
                moved = true;
            } else {
                // revert to the original angle and x position
                curBlock->angle -= deltaAngle;
                curBlock->x -= trans.x;
                curBlock->y -= deltaY;
            }
        } else {
            // record the fact that this block rotated
            moved = true;
        }
    }
   
    // if the block moved at all, its local sticking frames are reset
    // also, recalculate the number of squares this block can drop
    if (moved == true) {
        curBlock->localStickFrames = MAXLOCALSTICKFRAMES;
        curBlock->rowsDropped = calculateRowsDropped(curBlock);
    }

    if (curBlock->rowsDropped <= 0) {
        // block cannot drop - start to stick
        curBlock->globalStickFrames--;
        if (moved == false) 
            curBlock->localStickFrames--;
    } else {
        // the obstacle is no longer there - reset stick frames, and move down if required
        curBlock->globalStickFrames = MAXGLOBALSTICKFRAMES;
        curBlock->localStickFrames = MAXLOCALSTICKFRAMES;
        curBlock->y += trans.y;
        curBlock->rowsDropped -= trans.y;
    }

    // if the block has no stick frames left, place it down
    if ((curBlock->globalStickFrames <= 0) or (curBlock->localStickFrames <= 0)) {
        placeBlock(curBlock);
        delete curBlock;
        curBlock = NULL;
    }
}

// the shoveaway is a desperate attempt to rotate the block around obstacles
bool Board::shoveaway(Block* block) {
    int dir;

    // don't shoveaway a non-existent block
    if (block == NULL)
        return false;

    // attempt to rotate the block and possibly translate it
    for (int i = 0; i < 3; i++) {
        // the block can be shifted up to 2 units up in a shoveaway
        if (checkBlock(block) == OK) {
            return true;
        } else {
            // the block can also be shifted 1 unit left or right
            // to avoid giving preference to either direction, we decide randomly which one
            // to try first
            dir = 1 - 2*(rand()%2);
            block->x += dir;
            // if either direction works, we return the shoveaway
            if (checkBlock(block) == OK)
                return true;
            block->x -= 2*dir;
            if (checkBlock(block) == OK)
                return true;
            // otherwise, move back to center and shift up again 
            block->x += dir;
            block->y--;
        }
    }
    // at the end of the loop, the block has been moved up 3 squares - move it back down
    // no safe position was found, so the shoveaway fails
    block->y += 3;
    return false;
}

// place a block on the board, in its new fixed position
void Board::placeBlock(Block* block) {
    Point point;

    // log this event in multiplayer games
    if (gameMode == MULTIPLAYER) {
        events.push_back(PLACEBLOCK);
        events.push_back(block->x);
        events.push_back(block->y);
        events.push_back(block->angle);
    }

    // don't place a NULL block
    if (block == NULL)
        return;

    for (int i = 0; i < block->numSquares; i++) {
        // change square coordinates, from local coordinates into global
        if (block->angle%2 == 0) {
            // the block is rotated either 0 or 180 degrees
            point.x = block->x + block->squares[i].x*(1-(block->angle%4));
            point.y = block->y + block->squares[i].y*(1-(block->angle%4));
        } else {
            // the block is rotated either 90 or 270 degrees
            point.x = block->x + block->squares[i].y*((block->angle%4)-2);
            point.y = block->y + block->squares[i].x*(2-(block->angle%4));
        }
        board[point.x][point.y] = block->color;
        blocksInRow[point.y]++;
        if (point.y < highestRow)
            highestRow = point.y;
        boardChanged = true;
    }

    // check if any rows have to be removed
    int rowsCleared = removeRows();
    if ((gameMode == MULTIPLAYER) and (rowsCleared > 0)) {
        // in a multiplayer game, log the appropriate attack and show an animation
        events.push_back(SENDATTACK);
        events.push_back(rowsCleared + combo - 1);
        //if (drawSprites) startAnimation(block, rowsCleared+combo-1);
    }
}

void Board::getNextBlock(bool swap) {
    int b;

    // log this event in multiplayer games, except when it is called from queueBlock
    // when curBlockType == -1, this method was called from queueBlock, which is already logged
    if (gameMode == MULTIPLAYER) {
        events.push_back(GETNEXTBLOCK);
        events.push_back(swap ? 1 : 0);
    }

    if ((swap == false) or (heldBlockType == -1)) {
        // get the first element from the preview list - it is the new block
        b = preview.front();
        preview.pop_front();

        if (swap == true) {
            heldBlockType = curBlockType;
        } 
        // make the preview scroll to the next block
        previewAnim = PREVIEWANIMFRAMES;
        previewOffset = (blockData[b]->height+1)*squareWidth/2;
    } else {
        // user swapped out block - do not change the preview list
        b = heldBlockType;
        // hold the current block
        heldBlockType = curBlockType;
    }

    // record the new block type
    curBlockType = b;

    curBlock = new Block();
    curBlock->x = blockData[b]->x;
    curBlock->y = blockData[b]->y - blockData[b]->height + MAXBLOCKSIZE;
    curBlock->height = blockData[b]->height;
    curBlock->numSquares = blockData[b]->numSquares;
    oldBlock->numSquares = blockData[b]->numSquares;
    for (int i = 0; i < curBlock->numSquares; i++) {
        curBlock->squares[i] = Point(blockData[b]->squares[i].x, blockData[b]->squares[i].y);
        oldBlock->squares[i] = Point(blockData[b]->squares[i].x, blockData[b]->squares[i].y);
    }
    curBlock->color = blockData[b]->color;
    curBlock->rotates = blockData[b]->rotates;

    curBlock->rowsDropped = calculateRowsDropped(curBlock);
    if (curBlock->rowsDropped < 0) {
        boardState = GAMEOVER;
        if (drawSprites) gameover->frameCol = 1;
    }
    
    if (swap == false) {
        // if we just generated a new block, we can hold again
        holdUsed = false;
    } else {
        holdUsed = true;
        justHeld = true;
    }
}

int Board::checkBlock(Block* block) {
    Point point;
    int illegality = 0;
    int overlapsFound = 0;

    // don't check a non-existent block
    if (block == NULL)
        return OK;

    // run through each square to see if the block is in a legal position
    for (int i = 0; i < block->numSquares; i++) {
        // change square coordinates, from local coordinates into global
        if (block->angle%2 == 0) {
            // the block is rotated either 0 or 180 degrees
            point.x = block->x + block->squares[i].x*(1-(block->angle%4));
            point.y = block->y + block->squares[i].y*(1-(block->angle%4));
        } else {
            // the block is rotated either 90 or 270 degrees
            point.x = block->x + block->squares[i].y*((block->angle%4)-2);
            point.y = block->y + block->squares[i].x*(2-(block->angle%4));
        }
       
        // check for various illegalities with respect to the edges of the board
        if (point.y < 0) {
            // the errors with highest priority are being off the top or
            // bottom edge of the board
            if (illegality == 0)
                illegality = TOPEDGE;
        } else if (point.y >= NUMROWS) {
            // bottom edge - this can cause the block to stick
            if (illegality == 0)
                illegality = BOTTOMEDGE;
        } else if (point.x < 0) {
            // block is off the left edge of the board
            if (illegality == 0)
                illegality = LEFTEDGE;
        } else if (point.x >= NUMCOLS) {
            // same on the right
            if (illegality == 0)
                illegality = RIGHTEDGE;
        } else if (board[point.x][point.y] != BLACK) {
            // keep track of the number of overlaps with blocks already placed
            overlapsFound++;
        }
    }

    // the flag returned contains all the information found
    // flag%OVERLAP gives any edges the block strayed over
    // flag/OVERLAP gives the number of overlaps
    // if flag == OK (OK = 0) then the position is legal
    return illegality + OVERLAP*overlapsFound;
}

int Board::calculateRowsDropped(Block* block) {
    // don't do anything for a non-existent block
    if (block == NULL)
        return 0;

    for (int i = 0; i < NUMROWS+1; i++) {
        // check if the block is in a legal position
        if (checkBlock(block) == OK) {
            // still legal - move the block down 1 unit
            block->y++;
        } else {
            // the block is in illegal position - move it back, and
            // return the number of squares it can move down legally
            block->y -= i;
            // if the block is initially in an illegal position, return -1
            return i-1;
        }
    } 
}

// this method is called each time a block is placed - it clears any full rows
int Board::removeRows() {
    int downShift = 0;

    for (int y = NUMROWS-1; y >= highestRow; y--) {
        if (blocksInRow[y] == NUMCOLS) {
            // downShift keeps track of the number of cleared rows up to this point
            downShift++;
        } else if (downShift > 0) {
            // down shift this row by downShift rows
            for (int x = 0; x < NUMCOLS; x++) {
                board[x][y+downShift] = board[x][y];
                blocksInRow[y+downShift] = blocksInRow[y];
            }
        }
    }
    // if any rows were removed, add empty space to the top of the board
    if (downShift > 0) {
        for (int y = highestRow; y < highestRow+downShift; y++) {
            for (int x = 0; x < NUMCOLS; x++) {
                board[x][y] = BLACK;
                blocksInRow[y] = 0;
            }
        }
        highestRow += downShift;
        score += ((1<<downShift)-1);
        combo++; 
    } else combo = 0;

    return downShift;
}

/** Drawing routines start here!                            *
 *  The only public method is draw(SDL_Surface* sfc),       *
 *  which calls the private methods drawBoard, drawGUI,     *
 *  drawBlock, etc.                                        **/

void Board::draw(SDL_Surface* sfc) {
    if (boardState == PLAYING) {
        if (boardChanged == true) {
            redrawBoard(sfc);    
            boardChanged = false;
        } else {
            drawBoard(sfc);
        }
        justHeld = false;
    } else if ((boardState >= PAUSED) and (boardState < GAMEOVER)) {
        if (boardState == PAUSED) {
            redrawBoard(sfc);
            if (gameMode == SINGLEPLAYER) paused->draw(sfc);
            boardState++;
        }
    } else if ((boardState >= GAMEOVER) and (boardState < COUNTDOWN)) {
        if (boardState == GAMEOVER) {
            redrawBoard(sfc, true);
            if (gameMode == SINGLEPLAYER) {
                gameover->draw(sfc);
                boardState++;
            } else boardState = COUNTDOWN;
        }
    } else if (boardState >= COUNTDOWN) {
        if (drawSprites) {
            if (boardState%SECOND == COUNTDOWN) {
                gameover->frameRow = 1 + (boardState - COUNTDOWN)/SECOND;
                gameover->draw(sfc);
            }
            boardState++;
            if (boardState == NUMSECONDS*SECOND + COUNTDOWN) resetBoard();
        }
    }
}

void Board::drawBoard(SDL_Surface* sfc) {
    //if ((gameMode == MULTIPLAYER) and (drawSprites)) eraseAnimations(sfc);

    if (justHeld == true) {
        int xOffset = oldBlock->x-blockData[heldBlockType]->x;
        int yOffset = oldBlock->y-blockData[heldBlockType]->y;

        drawBlock(sfc, blockData[heldBlockType], true, false, xOffset, yOffset, oldBlock->angle);
        drawBlock(sfc, blockData[heldBlockType], true, false, xOffset, yOffset+oldBlock->rowsDropped, oldBlock->angle);
    } else {
        drawBlock(sfc, oldBlock, true, false, 0, 0, 0);
        drawBlock(sfc, oldBlock, true, false, 0, oldBlock->rowsDropped, 0);
    }

    if (curBlock != NULL) {
        drawBlock(sfc, curBlock, false, true, 0, curBlock->rowsDropped);
        drawBlock(sfc, curBlock);

        oldBlock->x = curBlock->x;
        oldBlock->y = curBlock->y;
        oldBlock->angle = curBlock->angle;
        oldBlock->rowsDropped = curBlock->rowsDropped;
    }

    drawGUI(sfc);
    
    //if ((gameMode == MULTIPLAYER) and (drawSprites)) drawAnimations(sfc);
}

void Board::redrawBoard(SDL_Surface* sfc, bool tinted, unsigned int tint) {
    unsigned int backColor, lineColor;

    //if ((gameMode == MULTIPLAYER) and (drawSprites)) eraseAnimations(sfc);

    backColor = BLACK;
    lineColor = mixedColor(WHITE, BLACK, LAMBDA);
    // on game over the screen reddens
    if (tinted == true) {
        backColor = mixedColor(tint, backColor, LAMBDA);
        lineColor = mixedColor(tint, lineColor, LAMBDA);
        curBlock->color = mixedColor(tint, curBlock->color, LAMBDA);
        highestRow = 0;
    }

    // first clear the board with black
    SDL_FillRectangle(sfc, xPos, yPos, xPos+squareWidth*NUMCOLS, yPos, boardHeight, BLACK);

    // draw in the vertical grid lines
    for (int i = 0; i < NUMCOLS; i++) {
        SDL_DrawLine(sfc, xPos+squareWidth*i, yPos, xPos+squareWidth*i, yPos+boardHeight-1, lineColor);
        SDL_DrawLine(sfc, xPos+squareWidth*(i+1)-1, yPos, xPos+squareWidth*(i+1)-1, yPos+boardHeight-1, lineColor);
    }

    // draw in the horizontal grid lines
    for (int i = 0; i < NUMROWS-MAXBLOCKSIZE+1; i++) {
        SDL_DrawLine(sfc, xPos, yPos+squareWidth*i, xPos+squareWidth*NUMCOLS-1, yPos+squareWidth*i, lineColor);
        SDL_DrawLine(sfc, xPos, yPos+squareWidth*(i+1)-1, xPos+squareWidth*NUMCOLS-1, yPos+squareWidth*(i+1)-1, lineColor);
    }
    // below the highest row, fill in the colors of the blocks there
    for (int y = highestRow; y < NUMROWS; y++) {
        for (int x = 0; x < NUMCOLS; x++) {
            if (tinted == true) {
                drawSquare(sfc, x, y, mixedColor(tint, board[x][y], 3*LAMBDA), (blocksInRow[y] == NUMCOLS));
            } else {
                drawSquare(sfc, x, y, board[x][y], (blocksInRow[y] == NUMCOLS));
            }
        }
    }

    if (curBlock != NULL) {
        drawBlock(sfc, curBlock, false, true, 0, curBlock->rowsDropped);
        drawBlock(sfc, curBlock);
    }

    drawGUI(sfc, tinted, tint);
    
    //if ((gameMode == MULTIPLAYER) and (drawSprites)) drawAnimations(sfc, tinted, tint);
}

void Board::drawGUI(SDL_Surface* sfc, bool tinted, unsigned int tint) {
    int listY, digit, i;
    int x = 1;
    int yQueue = 5*(squareWidth/2)*(PREVIEW+2);

    // if the board is tinted, we erase the entire GUI right here
    if (tinted == true) {
        if (gameMode == SINGLEPLAYER) numbers->frameRow = 2;
        SDL_FillRectangle(sfc, xPos+squareWidth*NUMCOLS, yPos, xPos+boardWidth, yPos, boardHeight-1, mixedColor(tint, BLACK, 2*LAMBDA));
    } else {
        if (gameMode == SINGLEPLAYER) numbers->frameRow = 1;
        // otherwise, if the preview is scrolling, then we erase the GUI
        if (previewAnim > 0) 
            SDL_FillRectangle(sfc, xPos+squareWidth*NUMCOLS, yPos, xPos+boardWidth, yPos, yQueue+1, BLACK);
    }

    // d acts like a y-Offset for the blocks - increases as we go down the preview
    if ((previewAnim > 0) || (tinted == true)) {
        list<int>::iterator j;
        int xOffset = squareWidth*NUMCOLS + sideBoard/2 - 3*squareWidth/4;

        j = preview.begin();
        // if the list is scrolling upwards, start at the next j and have a yOffset for the list 
        listY = 0;
        if (previewAnim > 0) listY = (previewOffset*(previewAnim-1))/PREVIEWANIMFRAMES;
    
        for (; j != preview.end(); j++) {
            // go through the preview list and draw each block
            if (listY == 0) {
                // the first one is drawn in a bright color
                drawSmallBlock(sfc, blockData[*j], xOffset, squareWidth+listY, squareWidth/2, -LAMBDA, tinted, tint);
            } else {
                // all others are drawn in dull colors
                drawSmallBlock(sfc, blockData[*j], xOffset, squareWidth+listY, squareWidth/2, 3*LAMBDA, tinted, tint);
            }
            listY += (blockData[*j]->height+2)*squareWidth/2;
        }    
    }
    if ((holdUsed == true) || (previewAnim > 0) || (tinted == true)) {
        // the following code executes when the held piece changes 
        if (tinted == true) {
            SDL_FillRectangle(sfc, xPos+squareWidth*NUMCOLS, yPos+yQueue+1, 
                    xPos+boardWidth, yPos+yQueue+1, boardHeight-yQueue-2, mixedColor(tint, BLACK, 2*LAMBDA));
        } else {
            SDL_FillRectangle(sfc, xPos+squareWidth*NUMCOLS, yPos+yQueue+1, 
                    xPos+boardWidth, yPos+yQueue+1, boardHeight-yQueue-2, BLACK);      
        }
        drawHold(sfc, holdUsed, tinted, tint);
        
        previewAnim--;
        if (previewAnim == 0)
            previewOffset = 0;
    } else {
        // when the GUI isn't changing, other than the score, just clear a small rectangle where the score/attack queue will be drawn
        if (gameMode == SINGLEPLAYER)
            SDL_FillRectangle(sfc, xPos+boardWidth-32-squareWidth/2, yPos+boardHeight-squareWidth/2-10, 
                    xPos+boardWidth-squareWidth/2, yPos+boardHeight-squareWidth/2-10, 10, BLACK);
        else 
            SDL_FillRectangle(sfc, xPos+squareWidth*NUMCOLS, yPos+boardHeight-squareWidth/2-6*ATTACKSIZE-1, 
                    xPos+boardWidth, yPos+boardHeight-squareWidth/2-6*ATTACKSIZE-1, 5*ATTACKSIZE, BLACK);
    }

    if (gameMode == SINGLEPLAYER) {
        // draw the score by picking the appropriate tiles from numbers
        numbers->x = xPos + boardWidth - squareWidth/2 - 8;
        numbers->y = yPos + boardHeight - squareWidth/2 - 10;
        for (int i = 0; i < 4; i++) {
            if ((score >= x) or (i == 0)) {
                digit = (score%(10*x))/x;
                numbers->frameCol = digit+1;
                numbers->draw(sfc);
                numbers->x -= 8;
            }
            x = 10*x;
        }
    } else if (attacks.size() > 0) {
        int xOffset = boardWidth - squareWidth/2 - 5*ATTACKSIZE;
        int yOffset = boardHeight - squareWidth/2 - 6*ATTACKSIZE;
        list<int>::iterator i = attacks.begin();
        drawSmallBlock(sfc, blockData[*i+numBlocks], xOffset + 9*ATTACKSIZE/2, yOffset, ATTACKSIZE, LAMBDA);
        int numDrawn = 1;

        for (i++; i != attacks.end(); i++) {
            drawSmallBlock(sfc, blockData[*i+numBlocks], xOffset, yOffset, ATTACKSIZE, 3*LAMBDA);
            numDrawn++;
            xOffset -= 9*ATTACKSIZE/2;
            if (xOffset < squareWidth*NUMCOLS + 6*ATTACKSIZE) {
                if (numDrawn == attacks.size()-1) {
                    drawSmallBlock(sfc, blockData[*(++i)+numBlocks], xOffset, yOffset, ATTACKSIZE, 3*LAMBDA);
                } else if (numDrawn < attacks.size()-1) {
                    drawSmallBlock(sfc, blockData[10+numBlocks], xOffset, yOffset, ATTACKSIZE, 2*LAMBDA);
                    drawSmallBlock(sfc, blockData[11+numBlocks], xOffset, yOffset, ATTACKSIZE, 2*LAMBDA);
                }
                break;
            } 
        }
    }
}

void Board::drawHold(SDL_Surface* sfc, bool shadow, bool tinted, unsigned int tint) {
    float lambda;
    
    if (shadow == true) {
        // draw the hold rectangle in the GUI to the right, in shadow - signifying that the hold has been used for this block 
        lambda = 0.7f;
    } else {
        // draw the hold rectangle in the GUI to the right in white 
        lambda = 0.0f;
    }
    int xOffset = xPos + squareWidth*NUMCOLS+squareWidth/2;
    int yOffset = yPos + 5*(squareWidth/2)*(PREVIEW+2)+1;
    if (tinted == true) {
        SDL_FillRectangle(sfc, xOffset, yOffset, xOffset+5*squareWidth/2, yOffset, 4*squareWidth, mixedColor(tint, WHITE, 4*LAMBDA));
        SDL_FillRectangle(sfc, xOffset+1, yOffset+1, xOffset+5*squareWidth/2-1, yOffset+1, 4*squareWidth-2, mixedColor(tint, BLACK, 2*LAMBDA));
    } else {
        SDL_FillRectangle(sfc, xOffset, yOffset, xOffset+5*squareWidth/2, yOffset, 4*squareWidth, mixedColor(BLACK, WHITE, lambda));
        SDL_FillRectangle(sfc, xOffset+1, yOffset+1, xOffset+5*squareWidth/2-1, yOffset+1, 4*squareWidth-2, BLACK);
    }
    if (heldBlockType != -1) { 
        xOffset = squareWidth*NUMCOLS + sideBoard/2 - 3*squareWidth/4;
        yOffset += -yPos + 2*squareWidth - ((squareWidth/2)*blockData[heldBlockType]->height)/2;
        drawSmallBlock(sfc, blockData[heldBlockType], xOffset, yOffset, (squareWidth/2), lambda, tinted);
    }
}

void Board::drawBlock(SDL_Surface* sfc, Block* block, bool erase, bool shadow, int xOffset, int yOffset, int aOffset) {
    Point point;

    // don't draw a non-existent block
    if (block == NULL)
        return;

    // draw a block, square by square
    for (int i = 0; i < block->numSquares; i++) {
        if ((block->angle+aOffset)%2 == 0) {
            // either the block is unrotated, or rotated 180 degrees - x's correspond to x's
            point.x = block->x + block->squares[i].x*(1-((block->angle+aOffset)%4));
            point.y = block->y + block->squares[i].y*(1-((block->angle+aOffset)%4));
        } else {
            // the block is rotated 90 or 270 degrees - x's in local coordinates are y's in global coords
            point.x = block->x + block->squares[i].y*(((block->angle+aOffset)%4)-2);
            point.y = block->y + block->squares[i].x*(2-((block->angle+aOffset)%4));
        }

        if ((point.x+xOffset >= 0) and (point.x+xOffset < NUMCOLS) and (point.y+yOffset >= 0) and (point.y+yOffset < NUMROWS)) {
            // draw the block at its correct position
            // active blocks are drawn in a lighter color than placed blocks 
            if (shadow == false) {
                if (erase == false) {
                    drawSquare(sfc, point.x+xOffset, point.y+yOffset, mixedColor(WHITE, block->color, LAMBDA*(1-LAMBDA)));
                } else {
                    drawSquare(sfc, point.x+xOffset, point.y+yOffset, board[point.x+xOffset][point.y+yOffset]);
                }
            } else {
                drawSquare(sfc, point.x+xOffset, point.y+yOffset, block->color, true);
            }
        }
    }
}

void Board::drawSmallBlock(SDL_Surface* sfc, Block* block, int xOffset, int yOffset, int width, float lambda, bool tinted, unsigned int tint) {
    Point point;
    SDL_Rect square;
    
    // don't draw a non-existent block
    if (block == NULL)
        return;

    // draw a block, square by square
    for (int i = 0; i < block->numSquares; i++) {
        if (block->angle%2 == 0) {
            // either the block is unrotated, or rotated 180 degrees - x's correspond to x's
            point.x = block->x + block->squares[i].x*(1-(block->angle%4));
            point.y = block->y + block->squares[i].y*(1-(block->angle%4));
        } else {
            // the block is rotated 90 or 270 degrees - x's in local coordinates are y's in global coords
            point.x = block->x + block->squares[i].y*((block->angle%4)-2);
            point.y = block->y + block->squares[i].x*(2-(block->angle%4));
        }
        square.x = xPos + xOffset + (point.x-NUMCOLS/2+1)*width;
        square.y = yPos + yOffset + point.y*width;
        square.w = width;
        square.h = width;
        if (tinted == true) {
            SDL_FillRect(sfc, &square, mixedColor(tint, mixedColor(BLACK, block->color, lambda), 3*LAMBDA));
        } else {
            SDL_FillRect(sfc, &square, mixedColor(BLACK, block->color, lambda)); 
        }
    }
}

void Board::drawSquare(SDL_Surface* sfc, int x, int y, int color, bool shadow) {
    SDL_Rect square;
  
    // don't draw the first MAXBLOCKSIZE-1 rows
    // shift the other rows up
    y -= MAXBLOCKSIZE-1;
    if (y < 0)
        return;

    // draws a specific square
    // first position the drawing square around the border
    square.x = xPos + squareWidth*x;
    square.y = yPos + squareWidth*y;
    square.w = squareWidth;
    square.h = squareWidth;

    // draw the square's border, a mix of BLACK and the square's color
    if (shadow == false) {
        SDL_FillRect(sfc, &square, mixedColor(WHITE, color, LAMBDA)); 
    } else {
        // draw the gray border around a shadowed square
        SDL_FillRect(sfc, &square, mixedColor(WHITE, BLACK, LAMBDA)); 
    }

    // shrink the square by 1 on all four sides
    square.x += 1;
    square.y += 1;
    square.w -= 2;
    square.h -= 2;

    // draw the interior of the square in the color board[x][y]
    if (shadow == false) {
        SDL_FillRect(sfc, &square, color); 
    } else {
        // clear the area under a shadowed square
        SDL_FillRect(sfc, &square, BLACK); 
    }

    if (shadow == true) {
        // draw a sequence of diagonal lines to represent the shadow
        for (int i = 0; i < 2*squareWidth-1; i++) {
            if (((squareWidth*(x+y))+i)%4 == 0) {
                if (i < squareWidth) {
                    SDL_DrawLine(sfc, xPos+squareWidth*x, yPos+squareWidth*y+i, xPos+squareWidth*x+i, yPos+squareWidth*y, color);
                } else {
                    SDL_DrawLine(sfc, xPos+squareWidth*(x+1)-1, yPos+squareWidth*(y-1)+i+1, 
                            xPos+squareWidth*(x-1)+i+1, yPos+squareWidth*(y+1)-1, color);
                }
            }
        }    
    }    
}

void Board::startAnimation(Block* block, int num) {
    int x = squareWidth*block->x;
    int y = squareWidth*(block->y - MAXBLOCKSIZE + 1);
    startAnimation(x, y, num);
}

void Board::startAnimation(int x, int y, int num) {
    int index = -1;

    for (int i = 0; i < MAXANIMATIONS; i++) {
        if (animBlock[i]->shoveaways == 0) {
            animBlock[i]->shoveaways = 1;
            animBlock[i]->x = blockData[num+numBlocks]->x;
            animBlock[i]->y = blockData[num+numBlocks]->y;
            animBlock[i]->numSquares = blockData[num+numBlocks]->numSquares;
            for (int j = 0; j < animBlock[i]->numSquares; j++) {
                animBlock[i]->squares[j].x = blockData[num+numBlocks]->squares[j].x;
                animBlock[i]->squares[j].y = blockData[num+numBlocks]->squares[j].y;
            }
            animBlock[i]->color = WHITE;
           
            animation[i]->x = x + squareWidth/2 + ANIMSIZE + xPos;
            if (x > boardWidth/2) animation[i]->x -= 3*squareWidth/2 + 2*ANIMSIZE;
            animation[i]->y = y + squareWidth/2 + ANIMSIZE + yPos;
            if (y > boardHeight/2) animation[i]->y -= 3*squareWidth/2 + 2*ANIMSIZE;
            index = i;
            break;
        }
    }

    // make sure this animation doesn't overlap with other animations
    for (int i = 0; i < MAXANIMATIONS; i++) {
        if ((animBlock[i]->shoveaways > 0) and (i != index)) {
            if ((abs(animation[i]->x-animation[index]->x) < 2*squareWidth) 
                    and (abs(animation[i]->y-animation[index]->y) < 2*squareWidth)) {
                animation[index]->x += ((rand()%3)-1)*squareWidth;
                animation[index]->y += ((rand()%3)-1)*squareWidth;
                // found and resolved a collision: restart the loop
                i = -1;
            }
        }
    }
}

void Board::eraseAnimations(SDL_Surface* sfc) {
    for (int i = 0; i < MAXANIMATIONS; i++) {
        if (animBlock[i]->shoveaways > 1) {
            animation[i]->erase(sfc);
            if (animBlock[i]->shoveaways == ANIMFRAMES) animBlock[i]->shoveaways = 0;
        }
    }
}

void Board::drawAnimations(SDL_Surface* sfc, bool tinted, unsigned int tint) {
    for (int i = 0; i < MAXANIMATIONS; i++)
        if (animBlock[i]->shoveaways > 0) 
            animation[i]->saveUnder(sfc);

    for (int i = 0; i < MAXANIMATIONS; i++) {
        if (animBlock[i]->shoveaways > 0) {
            animation[i]->frameCol = (tinted ? 2 : 1);
            animation[i]->draw(sfc);
            int squares = animBlock[i]->numSquares;
            animBlock[i]->numSquares = min(animBlock[i]->shoveaways, squares);
            drawSmallBlock(sfc, animBlock[i], animation[i]->x+2*ANIMSIZE, animation[i]->y+ANIMSIZE, ANIMSIZE, 0, tinted, tint);
            animBlock[i]->numSquares = squares;
            animBlock[i]->shoveaways++;
        }
    }
}

Board::~Board() {
    for (int i = 0; i < MAXANIMATIONS; i++) {
        delete animBlock[i];
        delete animation[i];
    }
    delete moveDir;
    delete curBlock;
    delete oldBlock;
}

