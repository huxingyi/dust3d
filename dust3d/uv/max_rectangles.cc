/*
 *  Copyright (c) 2016-2021 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved. 
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:

 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.

 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 *  
 *  Based on the Public Domain MaxRectsBinPack.cpp source by Jukka Jylänki
 *  https://github.com/juj/RectangleBinPack/
 *
 *  Ported to C# by Sven Magnus
 *  This version is also public domain - do whatever you want with it.
 *
 */

#include <assert.h>
#include <dust3d/uv/max_rectangles.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace dust3d {
namespace uv {

#ifdef MAX
#undef MAX
#endif
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#ifdef MIN
#undef MIN
#endif
#define MIN(a, b) ((a) < (b) ? (a) : (b))

    typedef struct MaxRectanglesRect {
        struct MaxRectanglesRect* next;
        struct MaxRectanglesRect* prev;
        int rectOrder;
        int x;
        int y;
        int width;
        int height;
        int inFreeRectLink : 1;
        int inUsedRectLink : 1;
        int inInputRectLink : 1;
    } MaxRectanglesRect;

    typedef struct MaxRectanglesContext {
        int width;
        int height;
        int rectCount;
        MaxRectanglesSize* rects;
        MaxRectanglesPosition* layoutResults;
        enum MaxRectanglesFreeRectChoiceHeuristic method;
        int allowRotations : 1;
        MaxRectanglesRect* freeRectLink;
        MaxRectanglesRect* usedRectLink;
        MaxRectanglesRect* inputRectLink;
    } MaxRectanglesContext;

    static void addRectToLink(MaxRectanglesRect* rect, MaxRectanglesRect** link)
    {
        rect->prev = 0;
        rect->next = *link;
        if (*link) {
            (*link)->prev = rect;
        }
        *link = rect;
    }

    static void freeRectOnLine(MaxRectanglesRect* rect, long line)
    {
        free(rect);
    }

#define freeRect(rect) freeRectOnLine((rect), __LINE__)

    static void removeRectFromLink(MaxRectanglesRect* rect, MaxRectanglesRect** link)
    {
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

    static void freeLink(MaxRectanglesRect** link)
    {
        while (*link) {
            MaxRectanglesRect* willDel = *link;
            *link = willDel->next;
            freeRect(willDel);
        }
    }

    static void addRectToFreeRectLinkOnLine(MaxRectanglesContext* ctx,
        MaxRectanglesRect* rect, long line)
    {
        assert(!rect->inInputRectLink && !rect->inFreeRectLink && !rect->inUsedRectLink);
        rect->inFreeRectLink = 1;
        addRectToLink(rect, &ctx->freeRectLink);
    }

#define addRectToFreeRectLink(ctx, rect) addRectToFreeRectLinkOnLine((ctx), (rect), __LINE__)

    static void addRectToUsedRectLink(MaxRectanglesContext* ctx, MaxRectanglesRect* rect)
    {
        assert(!rect->inInputRectLink && !rect->inFreeRectLink && !rect->inUsedRectLink);
        rect->inUsedRectLink = 1;
        addRectToLink(rect, &ctx->usedRectLink);
    }

    static void addRectToInputRectLink(MaxRectanglesContext* ctx, MaxRectanglesRect* rect)
    {
        assert(!rect->inInputRectLink && !rect->inFreeRectLink && !rect->inUsedRectLink);
        rect->inInputRectLink = 1;
        addRectToLink(rect, &ctx->inputRectLink);
    }

    static void removeAndFreeRectFromUsedRectLink(MaxRectanglesContext* ctx,
        MaxRectanglesRect* rect)
    {
        assert(rect->inUsedRectLink);
        rect->inUsedRectLink = 0;
        removeRectFromLink(rect, &ctx->usedRectLink);
    }

    static void removeAndFreeRectFromFreeRectLink(MaxRectanglesContext* ctx,
        MaxRectanglesRect* rect)
    {
        assert(rect->inFreeRectLink);
        rect->inFreeRectLink = 0;
        removeRectFromLink(rect, &ctx->freeRectLink);
    }

    static void removeAndFreeRectFromInputRectLink(MaxRectanglesContext* ctx,
        MaxRectanglesRect* rect)
    {
        assert(rect->inInputRectLink);
        rect->inInputRectLink = 0;
        removeRectFromLink(rect, &ctx->inputRectLink);
    }

    static MaxRectanglesRect* createRectOnLine(int x, int y, int width, int height,
        int rectOrder, long line)
    {
        MaxRectanglesRect* rect = (MaxRectanglesRect*)calloc(1, sizeof(MaxRectanglesRect));
        if (!rect) {
            return 0;
        }
        rect->x = x;
        rect->y = y;
        rect->width = width;
        rect->height = height;
        rect->rectOrder = rectOrder;
        return rect;
    }

#define createRect(x, y, width, height, rectOrder) createRectOnLine((x), (y), (width), (height), (rectOrder), __LINE__)

    MaxRectanglesRect* findPositionForNewNodeBottomLeft(MaxRectanglesContext* ctx,
        int width, int height, int* bestY, int* bestX)
    {
        MaxRectanglesRect* loop;
        MaxRectanglesRect* bestNode = createRect(0, 0, 0, 0, 0);
        if (!bestNode) {
            return 0;
        }

        *bestY = INT_MAX;

        for (loop = ctx->freeRectLink; loop; loop = loop->next) {
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

    MaxRectanglesRect* findPositionForNewNodeBestShortSideFit(MaxRectanglesContext* ctx,
        int width, int height, int* bestShortSideFit, int* bestLongSideFit)
    {
        MaxRectanglesRect* loop;
        MaxRectanglesRect* bestNode = createRect(0, 0, 0, 0, 0);
        if (!bestNode) {
            return 0;
        }

        *bestShortSideFit = INT_MAX;

        for (loop = ctx->freeRectLink; loop; loop = loop->next) {
            // Try to place the rectangle in upright (non-flipped) orientation.
            if (loop->width >= width && loop->height >= height) {
                int leftoverHoriz = abs((int)loop->width - width);
                int leftoverVert = abs((int)loop->height - height);
                int shortSideFit = MIN(leftoverHoriz, leftoverVert);
                int longSideFit = MAX(leftoverHoriz, leftoverVert);

                if (shortSideFit < *bestShortSideFit || (shortSideFit == *bestShortSideFit && longSideFit < *bestLongSideFit)) {
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

                if (flippedShortSideFit < *bestShortSideFit || (flippedShortSideFit == *bestShortSideFit && flippedLongSideFit < *bestLongSideFit)) {
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

    MaxRectanglesRect* findPositionForNewNodeBestLongSideFit(MaxRectanglesContext* ctx,
        int width, int height, int* bestShortSideFit, int* bestLongSideFit)
    {
        MaxRectanglesRect* loop;
        MaxRectanglesRect* bestNode = createRect(0, 0, 0, 0, 0);
        if (!bestNode) {
            return 0;
        }

        *bestLongSideFit = INT_MAX;

        for (loop = ctx->freeRectLink; loop; loop = loop->next) {
            // Try to place the rectangle in upright (non-flipped) orientation.
            if (loop->width >= width && loop->height >= height) {
                int leftoverHoriz = abs((int)loop->width - width);
                int leftoverVert = abs((int)loop->height - height);
                int shortSideFit = MIN(leftoverHoriz, leftoverVert);
                int longSideFit = MAX(leftoverHoriz, leftoverVert);

                if (longSideFit < *bestLongSideFit || (longSideFit == *bestLongSideFit && shortSideFit < *bestShortSideFit)) {
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

                if (longSideFit < *bestLongSideFit || (longSideFit == *bestLongSideFit && shortSideFit < *bestShortSideFit)) {
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

    MaxRectanglesRect* findPositionForNewNodeBestAreaFit(MaxRectanglesContext* ctx,
        int width, int height, int* bestAreaFit, int* bestShortSideFit)
    {
        MaxRectanglesRect* loop;
        MaxRectanglesRect* bestNode = createRect(0, 0, 0, 0, 0);
        if (!bestNode) {
            return 0;
        }

        *bestAreaFit = INT_MAX;

        for (loop = ctx->freeRectLink; loop; loop = loop->next) {
            int areaFit = (int)loop->width * (int)loop->height - width * height;

            // Try to place the rectangle in upright (non-flipped) orientation.
            if (loop->width >= width && loop->height >= height) {
                int leftoverHoriz = abs((int)loop->width - width);
                int leftoverVert = abs((int)loop->height - height);
                int shortSideFit = MIN(leftoverHoriz, leftoverVert);

                if (areaFit < *bestAreaFit || (areaFit == *bestAreaFit && shortSideFit < *bestShortSideFit)) {
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

                if (areaFit < *bestAreaFit || (areaFit == *bestAreaFit && shortSideFit < *bestShortSideFit)) {
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
        int i2start, int i2end)
    {
        if (i1end < i2start || i2end < i1start)
            return 0;
        return MIN(i1end, i2end) - MAX(i1start, i2start);
    }

    static int contactPointScoreNode(MaxRectanglesContext* ctx, int x, int y,
        int width, int height)
    {
        MaxRectanglesRect* loop;
        int score = 0;

        if (x == 0 || x + width == ctx->width)
            score += height;
        if (y == 0 || y + height == ctx->height)
            score += width;

        for (loop = ctx->usedRectLink; loop; loop = loop->next) {
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

    MaxRectanglesRect* findPositionForNewNodeContactPoint(MaxRectanglesContext* ctx,
        int width, int height, int* bestContactScore)
    {
        MaxRectanglesRect* loop;
        MaxRectanglesRect* bestNode = createRect(0, 0, 0, 0, 0);
        if (!bestNode) {
            return 0;
        }

        *bestContactScore = -1;

        for (loop = ctx->freeRectLink; loop; loop = loop->next) {
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

    static int initContext(MaxRectanglesContext* ctx)
    {
        int i;
        MaxRectanglesRect* newRect = createRect(0, 0, ctx->width, ctx->height, 0);
        if (!newRect) {
            return -1;
        }
        addRectToFreeRectLink(ctx, newRect);
        for (i = 0; i < ctx->rectCount; ++i) {
            newRect = createRect(0, 0, ctx->rects[i].width,
                ctx->rects[i].height, i + 1);
            if (!newRect) {
                return -1;
            }
            addRectToInputRectLink(ctx, newRect);
        }
        return 0;
    }

    static void releaseContext(MaxRectanglesContext* ctx)
    {
        freeLink(&ctx->usedRectLink);
        freeLink(&ctx->freeRectLink);
        freeLink(&ctx->inputRectLink);
    }

    static float getOccupany(MaxRectanglesContext* ctx)
    {
        unsigned long long usedSurfaceArea = 0;
        MaxRectanglesRect* loop = ctx->usedRectLink;
        while (loop) {
            usedSurfaceArea += loop->width * loop->height;
            loop = loop->next;
        }
        return (float)usedSurfaceArea / (ctx->width * ctx->height);
    }

    MaxRectanglesRect* scoreRect(MaxRectanglesContext* ctx, int width, int height,
        enum MaxRectanglesFreeRectChoiceHeuristic method, int* score1, int* score2)
    {
        MaxRectanglesRect* newNode = 0;
        *score1 = INT_MAX;
        *score2 = INT_MAX;
        switch (method) {
        case kMaxRectanglesBestShortSideFit:
            newNode = findPositionForNewNodeBestShortSideFit(ctx, width, height,
                score1, score2);
            break;
        case kMaxRectanglesBottomLeftRule:
            newNode = findPositionForNewNodeBottomLeft(ctx, width, height,
                score1, score2);
            break;
        case kMaxRectanglesContactPointRule:
            newNode = findPositionForNewNodeContactPoint(ctx, width, height, score1);
            *score1 = -*score1; // Reverse since we are minimizing, but for contact point score bigger is better.
            break;
        case kMaxRectanglesBestLongSideFit:
            newNode = findPositionForNewNodeBestLongSideFit(ctx, width, height,
                score2, score1);
            break;
        case kMaxRectanglesBestAreaFit:
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

    int splitFreeNode(MaxRectanglesContext* ctx, MaxRectanglesRect* freeNode,
        MaxRectanglesRect* usedNode)
    {
        // Test with SAT if the rectangles even intersect.
        if (usedNode->x >= freeNode->x + freeNode->width || usedNode->x + usedNode->width <= freeNode->x || usedNode->y >= freeNode->y + freeNode->height || usedNode->y + usedNode->height <= freeNode->y) {
            return 0;
        }

        if (usedNode->x < freeNode->x + freeNode->width && usedNode->x + usedNode->width > freeNode->x) {
            // New node at the top side of the used node.
            if (usedNode->y > freeNode->y && usedNode->y < freeNode->y + freeNode->height) {
                MaxRectanglesRect* newNode = createRect(freeNode->x, freeNode->y,
                    freeNode->width, freeNode->height, 0);
                if (!newNode) {
                    return -1;
                }
                newNode->height = usedNode->y - newNode->y;
                addRectToFreeRectLink(ctx, newNode);
            }

            // New node at the bottom side of the used node.
            if (usedNode->y + usedNode->height < freeNode->y + freeNode->height) {
                MaxRectanglesRect* newNode = createRect(freeNode->x, freeNode->y,
                    freeNode->width, freeNode->height, 0);
                if (!newNode) {
                    return -1;
                }
                newNode->y = usedNode->y + usedNode->height;
                newNode->height = freeNode->y + freeNode->height - (usedNode->y + usedNode->height);
                addRectToFreeRectLink(ctx, newNode);
            }
        }

        if (usedNode->y < freeNode->y + freeNode->height && usedNode->y + usedNode->height > freeNode->y) {
            // New node at the left side of the used node.
            if (usedNode->x > freeNode->x && usedNode->x < freeNode->x + freeNode->width) {
                MaxRectanglesRect* newNode = createRect(freeNode->x, freeNode->y,
                    freeNode->width, freeNode->height, 0);
                if (!newNode) {
                    return -1;
                }
                newNode->width = usedNode->x - newNode->x;
                addRectToFreeRectLink(ctx, newNode);
            }

            // New node at the right side of the used node.
            if (usedNode->x + usedNode->width < freeNode->x + freeNode->width) {
                MaxRectanglesRect* newNode = createRect(freeNode->x, freeNode->y,
                    freeNode->width, freeNode->height, 0);
                if (!newNode) {
                    return -1;
                }
                newNode->x = usedNode->x + usedNode->width;
                newNode->width = freeNode->x + freeNode->width - (usedNode->x + usedNode->width);
                addRectToFreeRectLink(ctx, newNode);
            }
        }

        return 1;
    }

    static int isContainedIn(MaxRectanglesRect* a, MaxRectanglesRect* b)
    {
        return a->x >= b->x && a->y >= b->y && a->x + a->width <= b->x + b->width && a->y + a->height <= b->y + b->height;
    }

    static void pruneFreeList(MaxRectanglesContext* ctx)
    {
        MaxRectanglesRect* outerLoop = ctx->freeRectLink;
        while (outerLoop) {
            MaxRectanglesRect* innerLoop;
            MaxRectanglesRect* outer = outerLoop;
            outerLoop = outerLoop->next;
            innerLoop = outerLoop;
            while (innerLoop) {
                MaxRectanglesRect* inner = innerLoop;
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

    static int placeRect(MaxRectanglesContext* ctx, MaxRectanglesRect* rect)
    {
        MaxRectanglesRect* loop = ctx->freeRectLink;
        while (loop) {
            MaxRectanglesRect* freeNode = loop;
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

    static int startLayout(MaxRectanglesContext* ctx)
    {
        while (ctx->inputRectLink) {
            int bestScore1 = INT_MAX;
            int bestScore2 = INT_MAX;
            MaxRectanglesRect* bestNode = 0;
            MaxRectanglesRect* bestRect = 0;
            MaxRectanglesRect* loop = ctx->inputRectLink;
            while (loop) {
                int score1 = 0;
                int score2 = 0;
                MaxRectanglesRect* newRect = 0;
                newRect = scoreRect(ctx, loop->width, loop->height, ctx->method, &score1, &score2);
                if (!newRect) {
                    return -1;
                }
                if (score1 < bestScore1 || (score1 == bestScore1 && score2 < bestScore2)) {
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
                return -1;
            }
            if (bestNode->width != bestRect->width || bestNode->height != bestRect->height) {
                bestNode->rectOrder = -bestRect->rectOrder;
            } else {
                bestNode->rectOrder = bestRect->rectOrder;
            }
            placeRect(ctx, bestNode);
            removeAndFreeRectFromInputRectLink(ctx, bestRect);
        }
        return 0;
    }

    static void fillResults(MaxRectanglesContext* ctx)
    {
        MaxRectanglesRect* loop = ctx->usedRectLink;
        while (loop) {
            int index = abs(loop->rectOrder) - 1;
            MaxRectanglesPosition* result = &ctx->layoutResults[index];
            result->left = loop->x;
            result->top = loop->y;
            result->rotated = loop->rectOrder < 0;
            loop = loop->next;
        }
    }

    int maxRectangles(int width, int height, int rectCount, MaxRectanglesSize* rects,
        enum MaxRectanglesFreeRectChoiceHeuristic method, int allowRotations,
        MaxRectanglesPosition* layoutResults, float* occupancy)
    {
        MaxRectanglesContext contextStruct;
        MaxRectanglesContext* ctx = &contextStruct;
        memset(ctx, 0, sizeof(MaxRectanglesContext));
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

}
}
