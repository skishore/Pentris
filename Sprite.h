
using namespace std;

#include "SDL.h"
#include <algorithm>

class Sprite {
	public:
		// ctor() default
		Sprite();
		// dtor		
		~Sprite();
		// load a sprite after screen is inited (width, height, imageName)		
		void loadSprite(int, int, int, const char*, int, int, SDL_Surface*, int, int);
        void initBackBuffer(const char*);
        void setAlpha(int);
		// draw sprite to screen, erase sprite, and save ground under sprite		
		void draw(SDL_Surface*);
		void erase(SDL_Surface*);
		void saveUnder(SDL_Surface*);		
		// position
		int x, y;
        // size of the surface the sprite is drawn in
        int screenwidth, screenheight;
        // column and row of current sprite frame
		int frameCol, frameRow;
	private:
		// size and bitdepth
		int width, height, depth;
		// number of sprite anim frames
		int  numCols, numRows;
		// graphic surfaces
		SDL_Surface *temp, *sprite, *back;
        bool backInited;
		// drawing rects and positioning procedure
		SDL_Rect rSrc, rTarget;
		bool positionRects();
};
