
using namespace std;

#include <cstdlib>
#include <complex>

#define BITDEPTH 32
// the intensity of the shading
#define LAMBDA 0.10f

// color constants
#define BLACK       0x00000000
#define GRAY        0x80808000
#define WHITE       0xffffff00
#define RED         0x0000ff00
#define LIME        0x00ff0000
#define BLUE        0xff000000
#define CYAN        0xffff0000
#define PURPLE      0x80008000
#define YELLOW      0x00ffff00
#define ORANGE      0x00a5ff00
#define DARKORANGE  0x008cff00
#define ORANGERED   0x0045ff00
#define TAN         0x8cb4d200
#define SALMON      0x7280fa00
#define DARKRED     0x1111b200
#define PURPLERED   0x11008b00
#define XLIGHTBLUE  0xebce8700
#define LIGHTBLUE   0xe1694100
#define PURPLEBLUE  0x8b000000
#define HOTPINK     0xff00ff00
#define PLUM        0xdda0dd00
#define ORCHID      0xd670da00
#define DARKPINK    0xcc669900
#define TURQUOISE   0xccd14800
#define DARKGREEN   0xaab22000
#define GREEN       0x71b33c00
#define LIGHTGREEN  0x98fb9800
#define XXXLITEGRAY 0xdddddd00
#define XXLIGHTGRAY 0xcccccc00
#define XLIGHTGRAY  0xbbbbbb00
#define LIGHTGRAY   0xaaaaaa00
#define GOLD        0x00d7ff00
#define STAIRCASE   0x0b86b800

// returns a color given RGB values
unsigned int rgb(int red, int green, int blue) {
    return (blue<<24) + (green<<16) + (red<<8);
}

// mixes two 32-bit colors - the result should be color1*lambda + color2*(1-lambda)
unsigned int mixedColor(unsigned int color1, unsigned int color2, float lambda) {
    // calculate the new blue
    int blue = ((color1>>24)%256)*lambda + ((color2>>24)%256)*(1-lambda);
    // left-shift both colors by 8 to find the greens, then calculate new green
    int green = ((color1>>16)%256)*lambda + ((color2>>16)%256)*(1-lambda);
    // same for red
    int red = ((color1>>8)%256)*lambda + ((color2>>8)%256)*(1-lambda);

    blue = min(max(blue, 0), 255);
    green = min(max(green, 0), 255);
    red = min(max(red, 0), 255);

    // alpha, the first value, should always be 255
    return rgb(red, green, blue);
}

// takes an index and returns the corresponding color
unsigned int oldCode(int index) {
    switch (index) {
        case 0:
            return WHITE;
        case 1:
            return XXXLITEGRAY;
        case 2:
            return XXLIGHTGRAY;
        case 3:
            return YELLOW;
        case 4:
            return XLIGHTGRAY;
        case 5:
            return XLIGHTBLUE;
        case 6:
            return SALMON;
        case 7:
            return PLUM;
        case 8:
            return GOLD;
        case 9:
            return ORCHID;
        case 10:
            return LIGHTGREEN;
        case 11:
            return LIGHTGRAY;
        case 12:
            return LIGHTBLUE;
        case 13:
            return RED;
        case 14:
            return BLUE;
        case 15:
            return DARKRED;
        case 16:
            return PURPLERED;
        case 17:
            return PURPLEBLUE;
        case 18:
            return HOTPINK;
        case 19:
            return PURPLE;
        case 20:
            return TAN;
        case 21:
            return DARKORANGE;
        case 22:
            return DARKGREEN;
        case 23:
            return STAIRCASE;
        case 24:
            return ORANGERED;
        case 25:
            return TURQUOISE;
        case 26:
            return DARKPINK;
        case 27:
            return ORANGE;
        case 28:
            return GREEN;
    }
}

unsigned int difficultyColor(unsigned int oldColor, float difficulty) {
    int blue = (oldColor>>24)%256;
    int green = (oldColor>>16)%256;
    int red = (oldColor>>8)%256; 

    if (difficulty > 0) {
        unsigned int blueShift = rgb(green/2, red/2, 255);
        float lambda = (0.8f + difficulty)/2.0f;
        return mixedColor(RED, blueShift, lambda);
    } else {
        return rgb(green/3, (red+blue)/3, 255);
    }
}

unsigned int colorCode(int index) {
	return mixedColor(WHITE, oldCode(index), 0.4f);
}
