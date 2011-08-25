
using namespace std;

#include "Sprite.h"

Sprite::Sprite() {}

void Sprite::loadSprite(int a, int b, int d, const char* name, int cols, int rows, SDL_Surface* screen, int swidth, int sheight) {
	int colorKey;		
	Uint32 rmask, gmask, bmask, amask;

	width = a;
	height = b;
	depth = d;
    screenwidth = swidth;
    screenheight = sheight;
	x = 0;
	y = 0;
	frameCol = 1;
	frameRow = 1;
	numCols = cols;
	numRows = rows;
    backInited = false;

	// load sprite
	temp = SDL_LoadBMP(name);
	sprite = SDL_DisplayFormat(temp);
	SDL_FreeSurface(temp);
    // set sprite transparent color
	colorKey = SDL_MapRGB(screen->format, 255, 0, 255);
	SDL_SetColorKey(sprite, SDL_SRCCOLORKEY | SDL_RLEACCEL, colorKey);
	SDL_SetAlpha(sprite, 0, 0);
}

void Sprite::initBackBuffer(const char* backName) {
	if (backInited == false) {
        // load extra back buffer
        temp = SDL_LoadBMP(backName);
        back = SDL_DisplayFormat(temp);
        SDL_FreeSurface(temp);
        backInited = true;
    }
}

void Sprite::setAlpha(int alpha) {
    SDL_SetAlpha(sprite, SDL_SRCALPHA | SDL_RLEACCEL, alpha);
}

void Sprite::saveUnder(SDL_Surface* target) {
	if (positionRects() == true) {
		if (backInited) {
            SDL_BlitSurface(target, &rTarget, back, &rSrc);
        } else {
            //move frame to empty spot
            rSrc.x = rSrc.x + width*numCols;
            // blit to empty spot
            SDL_BlitSurface(target, &rTarget, sprite, &rSrc);
        }
	}
}

void Sprite::erase(SDL_Surface* target) {
	if (positionRects() == true) {
		if (backInited) {
            SDL_BlitSurface(back, &rSrc, target, &rTarget);
        } else { 
            //move frame to empty spot
            rSrc.x = rSrc.x + width*numCols;
            // erase screen
            SDL_BlitSurface(sprite, &rSrc, target, &rTarget);
        }
	}
}

void Sprite::draw(SDL_Surface* target) {
	if (positionRects() == true) {
		//move to correct anim frame		
		rSrc.x = rSrc.x + width*(frameCol - 1);
		rSrc.y = rSrc.y + height*(frameRow - 1);
		// blit to screen
		SDL_BlitSurface(sprite, &rSrc, target, &rTarget);
	}
}

bool Sprite::positionRects() {
	//don't draw if sprite is outside screen	
	if ((x + width < 0) or (x > screenwidth) or (y + height < 0) or (y > screenheight)) {
		return false;
	} else {
		//place destination rect, taking care around boundary
		rTarget.x = max(x, 0);
		rTarget.y = max(y, 0);
		rTarget.w = width + min(x, 0);
		rTarget.h = height + min(y, 0);

		//place source rect
		rSrc.x = rTarget.x - x;
		rSrc.y = rTarget.y - y;
		rSrc.w = rTarget.w;
		rSrc.h = rTarget.h;

		return true;
	}
}

Sprite::~Sprite() {
	SDL_FreeSurface(sprite);
	if (backInited == true) SDL_FreeSurface(back);
}
