
using namespace std;

#include <cstdlib>
#include <complex>

#define BITDEPTH 32
// the intensity of the shading
#define LAMBDA 0.15f

// color constants
#define BLACK       0x00000000
#define GRAY        0x00808080
#define WHITE       0x00ffffff
#define RED         0x00ff0000
#define LIME        0x0000ff00
#define BLUE        0x000000ff
#define CYAN        0x0000ffff
#define PURPLE      0x00800080
#define YELLOW      0x00ffff00
#define ORANGE      0x00ffa500
#define DARKORANGE  0x00ff8c00
#define ORANGERED   0x00ff4500
#define TAN         0x00d2b48c
#define SALMON      0x00fa8072
#define DARKRED     0x00b21111
#define PURPLERED   0x008b0011
#define XLIGHTBLUE  0x0087ceeb
#define LIGHTBLUE   0x004169e1
#define PURPLEBLUE  0x0000008b
#define HOTPINK     0x00ff00ff
#define PLUM        0x00dda0dd
#define ORCHID      0x00da70d6
#define DARKPINK    0x009966cc
#define TURQUOISE   0x0048d1cc
#define DARKGREEN   0x0020b2aa
#define GREEN       0x003cb371
#define LIGHTGREEN  0x0098fb98
#define XXXLITEGRAY 0x00dddddd
#define XXLIGHTGRAY 0x00cccccc
#define XLIGHTGRAY  0x00bbbbbb
#define LIGHTGRAY   0x00aaaaaa
#define GOLD        0x00ffd700
#define STAIRCASE   0x00b8860b

// returns a color given RGB values
unsigned int rgb(int red, int green, int blue) {
    return (255<<24) + (red<<16) + (green<<8) + (blue);
}

// mixes two 32-bit colors - the result should be color1*lambda + color2*(1-lambda)
unsigned int mixedColor(unsigned int color1, unsigned int color2, float lambda) {
    // calculate the new blue
    int blue = (color1%256)*lambda + (color2%256)*(1-lambda);
    // left-shift both colors by 8 to find the greens, then calculate new green
    int green = ((color1>>8)%256)*lambda + ((color2>>8)%256)*(1-lambda);
    // same for red
    int red = ((color1>>16)%256)*lambda + ((color2>>16)%256)*(1-lambda);

    blue = min(max(blue, 0), 255);
    green = min(max(green, 0), 255);
    red = min(max(red, 0), 255);

    // alpha, the first value, should always be 255
    return rgb(red, green, blue);
}

// takes an index and returns the corresponding color
unsigned int colorCode(int index) {
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
    int blue = oldColor%256;
    int green = (oldColor>>8)%256;
    int red = (oldColor>>16)%256; 

    if (difficulty > 0) {
        unsigned int blueShift = rgb(green/2, red/2, 255);
        float lambda = (0.8f + difficulty)/2.0f;
        return mixedColor(RED, blueShift, lambda);
    } else {
        return rgb(green/3, (red+blue)/3, 255);
    }
}

