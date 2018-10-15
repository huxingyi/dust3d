/*
 	Based on the Public Domain MaxRectsBinPack.cpp source by Jukka Jylänki
 	https://github.com/juj/RectangleBinPack/

 	Ported to C# by Sven Magnus
 	This version is also public domain - do whatever you want with it.

  Ported to C by huxingyi@msn.com
  This version is MIT Licensed.
*/
#include "maxrects.h"
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifdef MAX
#undef MAX
#endif
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#ifdef MIN
#undef MIN
#endif
#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct maxRectsRect {
  struct maxRectsRect *next;
  struct maxRectsRect *prev;
  int rectOrder;
  int x;
  int y;
  int width;
  int height;
  int inFreeRectLink:1;
  int inUsedRectLink:1;
  int inInputRectLink:1;
} maxRectsRect;

typedef struct maxRectsContext {
  int width;
  int height;
  int rectCount;
  maxRectsSize *rects;
  maxRectsPosition *layoutResults;
  enum maxRectsFreeRectChoiceHeuristic method;
  int allowRotations:1;
  maxRectsRect *freeRectLink;
  maxRectsRect *usedRectLink;
  maxRectsRect *inputRectLink;
} maxRectsContext;

static void addRectToLink(maxRectsRect *rect, maxRectsRect **link) {
  rect->prev = 0;
  rect->next = *link;
  if (*link) {
    (*link)->prev = rect;
  }
  *link = rect;
}

static void freeRectOnLine(maxRectsRect *rect, long line) {
  //printf("%s:%ld: 0x%08x\n", __FUNCTION__, line,
  //  (int)((char *)rect - (char *)0));
  free(rect);
}

#define freeRect(rect) \
  freeRectOnLine((rect), __LINE__)

static void removeRectFromLink(maxRectsRect *rect, maxRectsRect **link) {
  if (rect->prev) {
    rect->prev->next = rect->next;
  }
  if (rect->next) {
    rect->next->prev = rect->prev;
  }
  if (rect == *link) {
    *link = rect->next;
  }
  freeRect(rect);
}

static void freeLink(maxRectsRect **link) {
  while (*link) {
    maxRectsRect *willDel = *link;
    *link = willDel->next;
    freeRect(willDel);
  }
}

static void addRectToFreeRectLinkOnLine(maxRectsContext *ctx,
    maxRectsRect *rect, long line) {
  //printf("%s:%ld: 0x%08x\n", __FUNCTION__, line,
  //  (int)((char *)rect - (char *)0));
  assert(!rect->inInputRectLink && !rect->inFreeRectLink &&
    !rect->inUsedRectLink);
  rect->inFreeRectLink = 1;
  addRectToLink(rect, &ctx->freeRectLink);
}

#define addRectToFreeRectLink(ctx, rect) \
  addRectToFreeRectLinkOnLine((ctx), (rect), __LINE__)

static void addRectToUsedRectLink(maxRectsContext *ctx, maxRectsRect *rect) {
  assert(!rect->inInputRectLink && !rect->inFreeRectLink &&
    !rect->inUsedRectLink);
  rect->inUsedRectLink = 1;
  addRectToLink(rect, &ctx->usedRectLink);
}

static void addRectToInputRectLink(maxRectsContext *ctx, maxRectsRect *rect) {
  assert(!rect->inInputRectLink && !rect->inFreeRectLink &&
    !rect->inUsedRectLink);
  rect->inInputRectLink = 1;
  addRectToLink(rect, &ctx->inputRectLink);
}

static void removeAndFreeRectFromUsedRectLink(maxRectsContext *ctx,
    maxRectsRect *rect) {
  //printf("%s: 0x%08x\n", __FUNCTION__, (int)((char *)rect - (char *)0));
  assert(rect->inUsedRectLink);
  rect->inUsedRectLink = 0;
  removeRectFromLink(rect, &ctx->usedRectLink);
}

static void removeAndFreeRectFromFreeRectLink(maxRectsContext *ctx,
    maxRectsRect *rect) {
  //printf("%s: 0x%08x\n", __FUNCTION__, (int)((char *)rect - (char *)0));
  assert(rect->inFreeRectLink);
  rect->inFreeRectLink = 0;
  removeRectFromLink(rect, &ctx->freeRectLink);
}

static void removeAndFreeRectFromInputRectLink(maxRectsContext *ctx,
    maxRectsRect *rect) {
  //printf("%s: 0x%08x\n", __FUNCTION__, (int)((char *)rect - (char *)0));
  assert(rect->inInputRectLink);
  rect->inInputRectLink = 0;
  removeRectFromLink(rect, &ctx->inputRectLink);
}

static maxRectsRect *createRectOnLine(int x, int y, int width, int height,
    int rectOrder, long line) {
  maxRectsRect *rect = (maxRectsRect *)calloc(1, sizeof(maxRectsRect));
  if (!rect) {
    fprintf(stderr, "%s: %s\n", __FUNCTION__, "calloc failed");
    return 0;
  }
  rect->x = x;
  rect->y = y;
  rect->width = width;
  rect->height = height;
  rect->rectOrder = rectOrder;
  //printf("%s:%ld: 0x%08x\n", __FUNCTION__, line,
  //  (int)((char *)rect - (char *)0));
  return rect;
}

#define createRect(x, y, width, height, rectOrder) \
  createRectOnLine((x), (y), (width), (height), (rectOrder), __LINE__)

maxRectsRect *findPositionForNewNodeBottomLeft(maxRectsContext *ctx,
    int width, int height, int *bestY, int *bestX) {
  maxRectsRect *loop;
	maxRectsRect *bestNode = createRect(0, 0, 0, 0, 0);
  if (!bestNode) {
    return 0;
  }

	*bestY = INT_MAX;

	for(loop = ctx->freeRectLink; loop; loop = loop->next) {
		// Try to place the rectangle in upright (non-flipped) orientation.
		if (loop->width >= width && loop->height >= height) {
			int topSideY = (int)loop->y + height;
			if (topSideY < *bestY || (topSideY == *bestY && loop->x < *bestX)) {
				bestNode->x = loop->x;
				bestNode->y = loop->y;
				bestNode->width = width;
				bestNode->height = height;
				*bestY = topSideY;
				*bestX = (int)loop->x;
			}
		}
		if (ctx->allowRotations && loop->width >= height && loop->height >= width) {
			int topSideY = (int)loop->y + width;
			if (topSideY < *bestY || (topSideY == *bestY && loop->x < *bestX)) {
				bestNode->x = loop->x;
				bestNode->y = loop->y;
				bestNode->width = height;
				bestNode->height = width;
				*bestY = topSideY;
				*bestX = (int)loop->x;
			}
		}
	}
	return bestNode;
}

maxRectsRect *findPositionForNewNodeBestShortSideFit(maxRectsContext *ctx,
    int width, int height, int *bestShortSideFit, int *bestLongSideFit)  {
  maxRectsRect *loop;
	maxRectsRect *bestNode = createRect(0, 0, 0, 0, 0);
  if (!bestNode) {
    return 0;
  }

	*bestShortSideFit = INT_MAX;

	for(loop = ctx->freeRectLink; loop; loop = loop->next) {
		// Try to place the rectangle in upright (non-flipped) orientation.
		if (loop->width >= width && loop->height >= height) {
			int leftoverHoriz = abs((int)loop->width - width);
			int leftoverVert = abs((int)loop->height - height);
			int shortSideFit = MIN(leftoverHoriz, leftoverVert);
			int longSideFit = MAX(leftoverHoriz, leftoverVert);

			if (shortSideFit < *bestShortSideFit ||
          (shortSideFit == *bestShortSideFit &&
            longSideFit < *bestLongSideFit)) {
				bestNode->x = loop->x;
				bestNode->y = loop->y;
				bestNode->width = width;
				bestNode->height = height;
				*bestShortSideFit = shortSideFit;
				*bestLongSideFit = longSideFit;
			}
		}

		if (ctx->allowRotations && loop->width >= height && loop->height >= width) {
			int flippedLeftoverHoriz = abs((int)loop->width - height);
			int flippedLeftoverVert = abs((int)loop->height - width);
			int flippedShortSideFit = MIN(flippedLeftoverHoriz, flippedLeftoverVert);
			int flippedLongSideFit = MAX(flippedLeftoverHoriz, flippedLeftoverVert);

			if (flippedShortSideFit < *bestShortSideFit ||
          (flippedShortSideFit == *bestShortSideFit &&
            flippedLongSideFit < *bestLongSideFit)) {
				bestNode->x = loop->x;
				bestNode->y = loop->y;
				bestNode->width = height;
				bestNode->height = width;
				*bestShortSideFit = flippedShortSideFit;
				*bestLongSideFit = flippedLongSideFit;
			}
		}
	}
	return bestNode;
}

maxRectsRect *findPositionForNewNodeBestLongSideFit(maxRectsContext *ctx,
    int width, int height, int *bestShortSideFit, int *bestLongSideFit) {
  maxRectsRect *loop;
	maxRectsRect *bestNode = createRect(0, 0, 0, 0, 0);
  if (!bestNode) {
    return 0;
  }

	*bestLongSideFit = INT_MAX;

	for(loop = ctx->freeRectLink; loop; loop = loop->next) {
		// Try to place the rectangle in upright (non-flipped) orientation.
		if (loop->width >= width && loop->height >= height) {
			int leftoverHoriz = abs((int)loop->width - width);
			int leftoverVert = abs((int)loop->height - height);
			int shortSideFit = MIN(leftoverHoriz, leftoverVert);
			int longSideFit = MAX(leftoverHoriz, leftoverVert);

			if (longSideFit < *bestLongSideFit ||
          (longSideFit == *bestLongSideFit &&
            shortSideFit < *bestShortSideFit)) {
				bestNode->x = loop->x;
				bestNode->y = loop->y;
				bestNode->width = width;
				bestNode->height = height;
				*bestShortSideFit = shortSideFit;
				*bestLongSideFit = longSideFit;
			}
		}

		if (ctx->allowRotations && loop->width >= height && loop->height >= width) {
			int leftoverHoriz = abs((int)loop->width - height);
			int leftoverVert = abs((int)loop->height - width);
			int shortSideFit = MIN(leftoverHoriz, leftoverVert);
			int longSideFit = MAX(leftoverHoriz, leftoverVert);

			if (longSideFit < *bestLongSideFit ||
          (longSideFit == *bestLongSideFit &&
            shortSideFit < *bestShortSideFit)) {
				bestNode->x = loop->x;
				bestNode->y = loop->y;
				bestNode->width = height;
				bestNode->height = width;
				*bestShortSideFit = shortSideFit;
				*bestLongSideFit = longSideFit;
			}
		}
	}
	return bestNode;
}

maxRectsRect *findPositionForNewNodeBestAreaFit(maxRectsContext *ctx,
    int width, int height, int *bestAreaFit, int *bestShortSideFit) {
  maxRectsRect *loop;
	maxRectsRect *bestNode = createRect(0, 0, 0, 0, 0);
  if (!bestNode) {
    return 0;
  }

	*bestAreaFit = INT_MAX;

	for(loop = ctx->freeRectLink; loop; loop = loop->next) {
		int areaFit = (int)loop->width * (int)loop->height - width * height;

		// Try to place the rectangle in upright (non-flipped) orientation.
		if (loop->width >= width && loop->height >= height) {
			int leftoverHoriz = abs((int)loop->width - width);
			int leftoverVert = abs((int)loop->height - height);
			int shortSideFit = MIN(leftoverHoriz, leftoverVert);

			if (areaFit < *bestAreaFit ||
          (areaFit == *bestAreaFit && shortSideFit < *bestShortSideFit)) {
				bestNode->x = loop->x;
				bestNode->y = loop->y;
				bestNode->width = width;
				bestNode->height = height;
				*bestShortSideFit = shortSideFit;
				*bestAreaFit = areaFit;
			}
		}

		if (ctx->allowRotations && loop->width >= height && loop->height >= width) {
			int leftoverHoriz = abs((int)loop->width - height);
			int leftoverVert = abs((int)loop->height - width);
			int shortSideFit = MIN(leftoverHoriz, leftoverVert);

			if (areaFit < *bestAreaFit ||
          (areaFit == *bestAreaFit && shortSideFit < *bestShortSideFit)) {
				bestNode->x = loop->x;
				bestNode->y = loop->y;
				bestNode->width = height;
				bestNode->height = width;
				*bestShortSideFit = shortSideFit;
				*bestAreaFit = areaFit;
			}
		}
	}
	return bestNode;
}

/// Returns 0 if the two intervals i1 and i2 are disjoint, or the length of their overlap otherwise.
static int commonIntervalLength(int i1start, int i1end,
    int i2start, int i2end) {
	if (i1end < i2start || i2end < i1start)
		return 0;
	return MIN(i1end, i2end) - MAX(i1start, i2start);
}

static int contactPointScoreNode(maxRectsContext *ctx, int x, int y,
    int width, int height) {
  maxRectsRect *loop;
	int score = 0;

	if (x == 0 || x + width == ctx->width)
		score += height;
	if (y == 0 || y + height == ctx->height)
		score += width;

	for(loop = ctx->usedRectLink; loop; loop = loop->next) {
		if (loop->x == x + width || loop->x + loop->width == x)
			score += commonIntervalLength((int)loop->y,
        (int)loop->y + (int)loop->height,
        y, y + height);
		if (loop->y == y + height || loop->y + loop->height == y)
			score += commonIntervalLength((int)loop->x,
        (int)loop->x + (int)loop->width, x, x + width);
	}
	return score;
}

maxRectsRect *findPositionForNewNodeContactPoint(maxRectsContext *ctx,
    int width, int height, int *bestContactScore) {
  maxRectsRect *loop;
	maxRectsRect *bestNode = createRect(0, 0, 0, 0, 0);
  if (!bestNode) {
    return 0;
  }

	*bestContactScore = -1;

	for(loop = ctx->freeRectLink; loop; loop = loop->next) {
		// Try to place the rectangle in upright (non-flipped) orientation.
		if (loop->width >= width && loop->height >= height) {
			int score = contactPointScoreNode(ctx, (int)loop->x, (int)loop->y,
        width, height);
			if (score > *bestContactScore) {
				bestNode->x = (int)loop->x;
				bestNode->y = (int)loop->y;
				bestNode->width = width;
				bestNode->height = height;
				*bestContactScore = score;
			}
		}
		if (ctx->allowRotations && loop->width >= height && loop->height >= width) {
			int score = contactPointScoreNode(ctx, (int)loop->x, (int)loop->y,
        height, width);
			if (score > *bestContactScore) {
				bestNode->x = (int)loop->x;
				bestNode->y = (int)loop->y;
				bestNode->width = height;
				bestNode->height = width;
				*bestContactScore = score;
			}
		}
	}
	return bestNode;
}

static int initContext(maxRectsContext *ctx) {
  int i;
  maxRectsRect *newRect = createRect(0, 0, ctx->width, ctx->height, 0);
  if (!newRect) {
    fprintf(stderr, "%s: %s\n", __FUNCTION__, "createRect failed");
    return -1;
  }
  addRectToFreeRectLink(ctx, newRect);
  for (i = 0; i < ctx->rectCount; ++i) {
    newRect = createRect(0, 0, ctx->rects[i].width,
        ctx->rects[i].height, i + 1);
    if (!newRect) {
      fprintf(stderr, "%s: %s\n", __FUNCTION__, "createRect failed");
      return -1;
    }
    addRectToInputRectLink(ctx, newRect);
  }
  return 0;
}

static void releaseContext(maxRectsContext *ctx) {
  freeLink(&ctx->usedRectLink);
  freeLink(&ctx->freeRectLink);
  freeLink(&ctx->inputRectLink);
}

static float getOccupany(maxRectsContext *ctx) {
	unsigned long long usedSurfaceArea = 0;
  maxRectsRect *loop = ctx->usedRectLink;
	while (loop) {
    usedSurfaceArea += loop->width * loop->height;
    loop = loop->next;
  }
	return (float)usedSurfaceArea / (ctx->width * ctx->height);
}

maxRectsRect *scoreRect(maxRectsContext *ctx, int width, int height,
    enum maxRectsFreeRectChoiceHeuristic method, int *score1, int *score2) {
	maxRectsRect *newNode = 0;
	*score1 = INT_MAX;
	*score2 = INT_MAX;
	switch(method) {
		case rectBestShortSideFit:
      newNode = findPositionForNewNodeBestShortSideFit(ctx, width, height,
        score1, score2);
      break;
		case rectBottomLeftRule:
      newNode = findPositionForNewNodeBottomLeft(ctx, width, height,
        score1, score2);
      break;
		case rectContactPointRule:
      newNode = findPositionForNewNodeContactPoint(ctx, width, height, score1);
			*score1 = -*score1; // Reverse since we are minimizing, but for contact point score bigger is better.
			break;
		case rectBestLongSideFit:
      newNode = findPositionForNewNodeBestLongSideFit(ctx, width, height,
        score2, score1);
      break;
		case rectBestAreaFit:
      newNode = findPositionForNewNodeBestAreaFit(ctx, width, height,
        score1, score2);
      break;
	}

	// Cannot fit the current rectangle.
	if (!newNode || 0 == newNode->height) {
		*score1 = INT_MAX;
		*score2 = INT_MAX;
	}

	return newNode;
}

int splitFreeNode(maxRectsContext *ctx, maxRectsRect *freeNode,
    maxRectsRect *usedNode) {
	// Test with SAT if the rectangles even intersect.
	if (usedNode->x >= freeNode->x + freeNode->width ||
      usedNode->x + usedNode->width <= freeNode->x ||
		  usedNode->y >= freeNode->y + freeNode->height ||
      usedNode->y + usedNode->height <= freeNode->y)
		return 0;

	if (usedNode->x < freeNode->x + freeNode->width &&
      usedNode->x + usedNode->width > freeNode->x) {
		// New node at the top side of the used node.
		if (usedNode->y > freeNode->y && usedNode->y <
        freeNode->y + freeNode->height) {
      maxRectsRect *newNode = createRect(freeNode->x, freeNode->y,
        freeNode->width, freeNode->height, 0);
			if (!newNode) {
        fprintf(stderr, "%s: %s\n", __FUNCTION__, "createRect failed");
        return -1;
      }
			newNode->height = usedNode->y - newNode->y;
      addRectToFreeRectLink(ctx, newNode);
		}

		// New node at the bottom side of the used node.
		if (usedNode->y + usedNode->height < freeNode->y + freeNode->height) {
      maxRectsRect *newNode = createRect(freeNode->x, freeNode->y,
        freeNode->width, freeNode->height, 0);
      if (!newNode) {
        fprintf(stderr, "%s: %s\n", __FUNCTION__, "createRect failed");
        return -1;
      }
			newNode->y = usedNode->y + usedNode->height;
			newNode->height = freeNode->y + freeNode->height -
        (usedNode->y + usedNode->height);
      addRectToFreeRectLink(ctx, newNode);
		}
	}

	if (usedNode->y < freeNode->y + freeNode->height &&
      usedNode->y + usedNode->height > freeNode->y) {
		// New node at the left side of the used node.
		if (usedNode->x > freeNode->x && usedNode->x <
        freeNode->x + freeNode->width) {
      maxRectsRect *newNode = createRect(freeNode->x, freeNode->y,
        freeNode->width, freeNode->height, 0);
      if (!newNode) {
        fprintf(stderr, "%s: %s\n", __FUNCTION__, "createRect failed");
        return -1;
      }
			newNode->width = usedNode->x - newNode->x;
			addRectToFreeRectLink(ctx, newNode);
		}

		// New node at the right side of the used node.
		if (usedNode->x + usedNode->width <
        freeNode->x + freeNode->width) {
      maxRectsRect *newNode = createRect(freeNode->x, freeNode->y,
        freeNode->width, freeNode->height, 0);
      if (!newNode) {
        fprintf(stderr, "%s: %s\n", __FUNCTION__, "createRect failed");
        return -1;
      }
			newNode->x = usedNode->x + usedNode->width;
			newNode->width = freeNode->x + freeNode->width -
        (usedNode->x + usedNode->width);
			addRectToFreeRectLink(ctx, newNode);
		}
	}

	return 1;
}

static int isContainedIn(maxRectsRect *a, maxRectsRect *b) {
	return a->x >= b->x && a->y >= b->y &&
		a->x + a->width <= b->x + b->width &&
		a->y + a->height <= b->y + b->height;
}

static void pruneFreeList(maxRectsContext *ctx) {
  maxRectsRect *outerLoop = ctx->freeRectLink;
  while (outerLoop) {
    maxRectsRect *innerLoop;
    maxRectsRect *outer = outerLoop;
    outerLoop = outerLoop->next;
    innerLoop = outerLoop;
    while (innerLoop) {
      maxRectsRect *inner = innerLoop;
      innerLoop = innerLoop->next;
      if (isContainedIn(outer, inner)) {
        removeAndFreeRectFromFreeRectLink(ctx, outer);
				break;
			}
      if (isContainedIn(inner, outer)) {
        if (inner == outerLoop) {
          outerLoop = outerLoop->next;
        }
        removeAndFreeRectFromFreeRectLink(ctx, inner);
      }
    }
  }
}

static int placeRect(maxRectsContext *ctx, maxRectsRect *rect) {
  maxRectsRect *loop = ctx->freeRectLink;
  while (loop) {
    maxRectsRect *freeNode = loop;
    int splitResult = splitFreeNode(ctx, freeNode, rect);
    if (splitResult < 0) {
      return -1;
    }
    loop = loop->next;
    if (splitResult) {
      removeAndFreeRectFromFreeRectLink(ctx, freeNode);
    }
  }
  pruneFreeList(ctx);
  addRectToUsedRectLink(ctx, rect);
  return 0;
}

static int startLayout(maxRectsContext *ctx) {
  while (ctx->inputRectLink) {
    int bestScore1 = INT_MAX;
		int bestScore2 = INT_MAX;
    maxRectsRect *bestNode = 0;
    maxRectsRect *bestRect = 0;
    maxRectsRect *loop = ctx->inputRectLink;
    while (loop) {
      int score1 = 0;
			int score2 = 0;
      maxRectsRect *newRect = 0;
      newRect = scoreRect(ctx, loop->width, loop->height, ctx->method,
          &score1, &score2);
      if (!newRect) {
        return -1;
      }
      if (score1 < bestScore1 ||
          (score1 == bestScore1 && score2 < bestScore2)) {
				bestScore1 = score1;
				bestScore2 = score2;
        if (bestNode) {
          freeRect(bestNode);
        }
        bestRect = loop;
				bestNode = newRect;
        newRect = 0;
			}
      if (newRect) {
        freeRect(newRect);
      }
      loop = loop->next;
    }
    if (!bestNode) {
      fprintf(stderr, "%s: %s\n", __FUNCTION__, "find bestRect failed");
      return -1;
    }
    if (bestNode->width != bestRect->width ||
        bestNode->height != bestRect->height) {
      bestNode->rectOrder = -bestRect->rectOrder;
    } else {
      bestNode->rectOrder = bestRect->rectOrder;
    }
    placeRect(ctx, bestNode);
    removeAndFreeRectFromInputRectLink(ctx, bestRect);
  }
  return 0;
}

static void fillResults(maxRectsContext *ctx) {
  maxRectsRect *loop = ctx->usedRectLink;
  while (loop) {
    int index = abs(loop->rectOrder) - 1;
    maxRectsPosition *result = &ctx->layoutResults[index];
    result->left = loop->x;
    result->top = loop->y;
    result->rotated = loop->rectOrder < 0;
    loop = loop->next;
  }
}

int maxRects(int width, int height, int rectCount, maxRectsSize *rects,
    enum maxRectsFreeRectChoiceHeuristic method, int allowRotations,
    maxRectsPosition *layoutResults, float *occupancy) {
  maxRectsContext contextStruct;
  maxRectsContext *ctx = &contextStruct;
  memset(ctx, 0, sizeof(maxRectsContext));
  ctx->width = width;
  ctx->height = height;
  ctx->method = method;
  ctx->allowRotations = allowRotations;
  ctx->rectCount = rectCount;
  ctx->rects = rects;
  ctx->layoutResults = layoutResults;
  if (0 != initContext(ctx)) {
    releaseContext(ctx);
    return -1;
  }
  if (0 != startLayout(ctx)) {
    releaseContext(ctx);
    return -1;
  }
  if (occupancy) {
    *occupancy = getOccupany(ctx);
  }
  fillResults(ctx);
  releaseContext(ctx);
  return 0;
}
