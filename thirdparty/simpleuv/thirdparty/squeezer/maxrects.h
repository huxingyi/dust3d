/*
 	Based on the Public Domain MaxRectsBinPack.cpp source by Jukka Jylänki
 	https://github.com/juj/RectangleBinPack/

 	Ported to C# by Sven Magnus
 	This version is also public domain - do whatever you want with it.

  Ported to C by huxingyi@msn.com
  This version is MIT Licensed.
*/

#ifndef MAX_RECTS_H
#define MAX_RECTS_H

typedef struct maxRectsSize {
  int width;
  int height;
} maxRectsSize;

typedef struct maxRectsPosition {
  int left;
  int top;
  int rotated:1;
} maxRectsPosition;

enum maxRectsFreeRectChoiceHeuristic {
  rectBestShortSideFit, ///< -BSSF: Positions the rectangle against the short side of a free rectangle into which it fits the best.
  rectBestLongSideFit, ///< -BLSF: Positions the rectangle against the long side of a free rectangle into which it fits the best.
  rectBestAreaFit, ///< -BAF: Positions the rectangle into the smallest free rect into which it fits.
  rectBottomLeftRule, ///< -BL: Does the Tetris placement.
  rectContactPointRule ///< -CP: Choosest the placement where the rectangle touches other rects as much as possible.
};

int maxRects(int width, int height, int rectCount, maxRectsSize *rects,
    enum maxRectsFreeRectChoiceHeuristic method, int allowRotations,
    maxRectsPosition *layoutResults, float *occupancy);

#endif
