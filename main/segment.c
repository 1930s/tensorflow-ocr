/*
segment.c: chop the raster into pieces.

Copyright © Raphael Finkel 2007-2010 raphael@cs.uky.edu  

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ocr.h"
#include <assert.h>
#include <math.h>
#define SPLITGAP (40) // larger means less gap

// for the world
int header, spacer, lineWidth;
int numLines;
int leftMargin, rightMargin, glyphWidth, glyphHeight;
lineHeaderList *lineHeaders; // with a dummy at the front

// for this file
float averageLineHeight;
int *copyPath(int *path, int height);
int *tryPathSeparation(glyph_t *glyphPtr);
int *buildSlantedPath(glyph_t *glyph, int xStartIndex);
void shiftRight(int *path, int height);
void finishPathSplit(glyph_t *glyphPtr, glyph_t *first, int *splitPath);
void freeGlyph(glyph_t *glyphPtr);
int *adjustPath(int *oldPath, int oldHeight, int stretchAbove,
	int stretchBelow);

#define ISBLANK(row, col) (image[AT(row,col)] == 0)
#define ISFILLED(row, col) (image[AT(row,col)])
#define HEIGHT(glyph) (glyph->bottom - glyph->top)

// returns if row is blank in range [startCol, endCol).
static int isBlankRow(row, startCol, endCol) {
	int col;
	for (col = startCol; col < endCol; col += 1) {
		if (image[AT(row, col)]) {
			// fprintf(stderr, "not blank row, as seen at %d,%d\n", row, col);
			return(false);
		}
	} // each col
	return(true);
} // isBlankRow

#define SKIPBLANKLINES \
	do { \
		for (; row < height; row += 1) { \
			if (!isBlankRow(row, 0, width)) break; \
		} \
	} while (0)

#define SKIPNONBLANKLINES \
	do { \
		for (; row < height; row += 1) { \
			if (isBlankRow(row, 0, width)) break; \
		} \
	} while (0)

#define SKIPBLANKCOLS \
	do { \
		for (; col < width; col += 1) { \
			if (!isBlankCol(top, bottom, col)) break; \
		} \
	} while (0)

#define SKIPNONBLANKCOLS \
	do { \
		for (; col < width; col += 1) { \
			if (isBlankCol(top, bottom, col)) break; \
		} \
	} while (0)

static int isBlankCol(int topRow, int bottomRow, int col) {
	int row;
	for (row = topRow; row < bottomRow; row += 1) {
		if (image[AT(row, col)]) {
			// fprintf(stderr, "not blank column: %d,%d\n", row, col);
			return(false);
		}
	} // each col
	return(true);
} // isBlankCol

static int countNonBlankCol(int topRow, int bottomRow, int col) {
	// return how many cells in column are not blank
	int answer = 0;
	int row;
	for (row = topRow; row < bottomRow; row += 1) {
		if (image[AT(row, col)]) {
			answer += 1;
		}
	} // each col
	return(answer);
} // countNonBlankCol

#ifdef undef
static int countBits(int topRow, int bottomRow, int col) {
	int answer = 0;
	int row;
	for (row = topRow; row < bottomRow; row += 1) {
		if (image[AT(row, col)]) answer += 1;
	} // each col
	return(answer);
} // countBits
#endif

void showGlyphs(textLine *theLine, const char *msg) { // debugging output
	fprintf(stderr, "%s: line at y values [%d,%d]\n", msg, theLine->top,
		theLine->bottom);
	glyph_t *theGlyph;
	for (theGlyph = theLine->glyphs->next; theGlyph; theGlyph = theGlyph->next) {
		int fillingNow = 0;
		if (!theGlyph->tuple) {
			theGlyph->tuple = newTuple();
			fillTuple(theGlyph, theGlyph->tuple);
			fillingNow = 1;
		}

		fprintf(stderr, "\tx=[%d,%d] y=[%d,%d] %s\n",
			theGlyph->left, theGlyph->right,
			theGlyph->top, theGlyph->bottom,
			ocrValue(theGlyph->tuple)
			);

		if (fillingNow) {
			free(theGlyph->tuple);
			theGlyph->tuple = NULL;
		}
	} // one line
	fprintf(stderr, "\n");
} // showGlyphs

void showLines(lineHeaderList *theLines) { // debugging output
	lineHeaderList *curLine;
	fprintf(stderr, "Current lines\n");
	int lineNumber = 0;
	for (curLine = theLines->next; curLine; curLine=curLine->next) {
		fprintf(stderr, "\tline between y=(%d,%d)\n", curLine->line->top,
			curLine->line->bottom);
		// showGlyphs(curLine->line, "contents");
		lineNumber += 1;
	} // one line
} // showLines

int totalWidth = 0, totalHeight = 0, glyphCount = 0;

glyph_t *insertGlyph(int left, int right, int top, int bottom, lineHeaderList *theLineList) {
	// right and bottom are 1 beyond the edge.
	// fprintf(stderr, "Inserting a glyph at (%d,%d)\n", left, top);
	// statistics
	leftMargin = MIN(leftMargin, left);
	rightMargin = MAX(rightMargin, right);
	totalWidth += right - left;
	totalHeight += bottom - top;
	if (bottom - top > maxGlyphHeight) {
		fprintf(stderr, "strange: total height is %d\n", totalHeight);
		abort();
	}
	glyphCount += 1;
	glyphWidth = totalWidth / glyphCount;
	glyphHeight = totalHeight / glyphCount;
	// allocate
	glyph_t *newGlyphPtr = (glyph_t *) calloc(1, sizeof(glyph_t));
	// initialize
	newGlyphPtr->left = left;
	newGlyphPtr->right = right;
	newGlyphPtr->top = top;
	newGlyphPtr->bottom = bottom;
	newGlyphPtr->lineHeight = 0; // filled in at a later step
	newGlyphPtr->distance = -1; // changed when we paint rectangle
	// we fill tuple, next, leftPath, rightPath later if at all.
	// place in appropriate line
	lineHeaderList *curLineHeader; // one before (above) the line we are looking at
	int success = 0;
	for (curLineHeader = theLineList; curLineHeader->next;
			curLineHeader = curLineHeader->next) { // each known line
		textLine *tryLine = curLineHeader->next->line; // the one we are looking at
		// fprintf(stderr, "trying a line between (%d,%d)\n", tryLine->top,
		// 	tryLine->bottom);
		if (tryLine->top > bottom) break; // we have gone too far; need new line
		int ok = false;
		// fprintf(stderr, "line %d-%d; glyph %d-%d: ",
		// 	tryLine->top, tryLine->bottom, top, bottom);
		// separating these cases for debugging purposes.
		if (tryLine->top <= top && tryLine->bottom > top) {
			// existing line contains the top of the glyph
			ok = true;
			// fprintf(stderr, "case 1\n");
		} else if (tryLine->top < bottom && tryLine->bottom >= bottom) {
			// existing line contains the bottom of the glyph
			ok = true;
			// fprintf(stderr, "case 2\n");
		} else if (tryLine->top >= top && tryLine->bottom <= bottom) {
			// existing line completely contained within the glyph
			ok = true;
			// fprintf(stderr, "case 3\n");
		} else {
			// fprintf(stderr, "\tnot on this line\n");
		}
		if (ok) {
		 	// appropriate line
			// fprintf(stderr, "line is appropriate\n");
			/* // make a new line if the glyph doesn't overlap this line well
			float overlap = MIN(tryLine->bottom - top, bottom - tryLine->top);
			if (overlap / HEIGHT(tryLine) < 0.3 &&
				overlap > (bottom-top)/2 ) {
				// too dissimilar to current line; use another or make new
				fprintf(stderr, "lines dissimilar\n");
				continue;
			}
			*/
			tryLine->top = MIN(tryLine->top, top);
			tryLine->bottom = MAX(tryLine->bottom, bottom);
			tryLine->leftBorder = MIN(tryLine->leftBorder, left);
			tryLine->rightBorder = MAX(tryLine->rightBorder, right);
			glyph_t *curGlyphPtr; // first entry in line is a dummy
			for (curGlyphPtr = tryLine->glyphs; curGlyphPtr->next; 
					curGlyphPtr = curGlyphPtr->next) { // each known glyph on line
				// fprintf(stderr, "trying a glyph at %d\n",
				//  	curGlyphPtr->next->left);
				if (curGlyphPtr->next->left > left) {
					newGlyphPtr->next = curGlyphPtr->next;
					// fprintf(stderr,
					// 	"position is appropriate: place [%d,%d] before [%d,%d]\n",
					// 	left, right, curGlyphPtr->next->left, curGlyphPtr->next->right);
					// fprintf(stderr, "distance from neigbor in x: %d y: %d\n",
					// 	newGlyphPtr->next->left - right,
					// 	MIN(abs(newGlyphPtr->next->bottom - bottom), 
					// 		abs(newGlyphPtr->next->top - top))
					// 	);
					break; // we found the right position in line
				}
			} // each known glyph on line
			curGlyphPtr->next = newGlyphPtr; // whether we fell off the end or not
			success = 1;
			break; // we found the right line
		} // tryLine is the appropriate line
	} // each known line
	if (!success) { // we need to start a new line
		// fprintf(stderr, "starting a new line\n");
		lineHeaderList *newLineHeader =
			(lineHeaderList *) malloc(sizeof(lineHeaderList));
		newLineHeader->next = curLineHeader->next; // even if empty
		curLineHeader->next = newLineHeader;
		newLineHeader->line = (textLine *) malloc(sizeof(textLine));
		newLineHeader->line->top = top;
		newLineHeader->line->bottom = bottom;
		newLineHeader->line->leftBorder = left;
		newLineHeader->line->rightBorder = right;
		newLineHeader->line->glyphs = (glyph_t *) calloc(1, sizeof(glyph_t));
			// dummy
		newLineHeader->line->glyphs->next = newGlyphPtr;
	} // starting a new line
	return newGlyphPtr;
	// fprintf(stderr, "exiting insertGlyph\n"); // breakpoint spot
	// showLines(); // debug
	// float distance;
	// const glyph_t *neighbor;
	// distance = p_closestGlyph(newGlyphPtr, &neighbor);
	// p_insertGlyph(newGlyphPtr);
} // insertGlyph

void findLinesHarder(int top, int bottom, int left, int right); // forward

void findCells(top, bottom, left, right) {
	// try to find regions within the given boundaries that are single glyphs
	// trim white space above.  bottom and right are at the borders, not beyond.
	// fprintf(stderr, "top %d bottom %d left %d right %d\n", top, bottom, left,
	// 	right);
	for (; left <= right; left += 1) {
		if (!isBlankCol(top, bottom, left)) break;
	}
	for (; left <= right; right -= 1) {
		if (!isBlankCol(top, bottom, right)) break;
	}
	for (; top <= bottom; top += 1) {
		if (!isBlankRow(top, left, right)) break;
	}
	for (; top <= bottom; bottom -= 1) {
		if (!isBlankRow(bottom, left, right)) break;
	}
	// fprintf(stderr, "after trim: top %d bottom %d left %d right %d\n", top,
	// 	bottom, left, right);
	int height = bottom+1-top;
	int width = right+1-left;
	if (width < minGlyphWidth || height < minGlyphHeight ||
			height * width < minGlyphArea) {
		// fprintf(stdout, "too small (%dx%d=%d); returning\n", right-left,
		// 	bottom-top, (bottom-top)*(right-left));
		return;
	}
	// look for horizontal white area as close to the middle as possible
	int mid = (bottom+1+top) / 2;
	for (; mid < bottom; mid += 1) {
		if (isBlankRow(mid, left, right)) break;
	}
	if (mid < bottom) {
		findCells(mid+1, bottom, left, right);
		// lower half first for efficiency
		findCells(top, mid-1, left, right);
		return;
	}
	for (; mid > top; mid -= 1) {
		if (isBlankRow(mid, left, right)) break;
	}
	if (mid > top) {
		findCells(mid+1, bottom, left, right);
		findCells(top, mid-1, left, right);
		return;
	}
	// look for vertical white area as close to the middle as possible
	mid = (right+left) / 2;
	for (; mid < right; mid += 1) {
		if (isBlankCol(top, bottom, mid)) break;
	}
	if (mid < right) {
		findCells(top, bottom, mid+1, right); // right half first for efficiency
		findCells(top, bottom, left, mid-1);
		return;
	}
	mid = (right+left) / 2;
	for (; mid > left; mid -= 1) {
		if (isBlankCol(top, bottom, mid)) break;
	}
	if (mid > left) {
		findCells(top, bottom, mid+1, right); // right half first for efficiency
		findCells(top, bottom, left, mid-1);
		return;
	}
	if ((right+1-left > maxGlyphWidth) || (bottom-top > maxGlyphHeight)) {
		// fprintf(stderr, "too large: from (%d,%d) to (%d,%d)\n",
		// 	left, top, right, bottom);
		findLinesHarder(top, bottom, left, right);
		return;
	}
	// fprintf(stderr, "glyph found, tight bounds x=[%d,%d], y=[%d,%d]\n",
	// 	left, right, top, bottom);
	insertGlyph(left, right+1, top, bottom+1, lineHeaders);
} // findCells

#define SOMEDARK 30 // amount of dark that is still white
void findLinesHarder(int top, int bottom, int left, int right)  {
	// this region is too large to be a glyph, but it might have multiple lines
	// without actual white space.
	int row, col;
	int minBlackCount = INFTY, minBlackRow;
	for (row = top; row < bottom; row += 1) {
		int darkCount = 0;
		for (col = left; col < right; col += 1) {
			if (ISFILLED(row, col)) {
				darkCount += 1;
			}
		} // each col
		if (darkCount < minBlackCount) {
			minBlackCount = darkCount;
			minBlackRow = row;
		}
	} // each row
	if (minBlackCount < SOMEDARK) {
		if (minBlackRow-1-top >= minGlyphHeight) {
			// fprintf(stderr, "\tsplit above row %d\n", minBlackRow);
			findCells(top, minBlackRow-1, left, right); 
		}
		if (bottom-(minBlackRow+1) >= minGlyphHeight) {
			// fprintf(stderr, "\tsplit below row %d\n", minBlackRow);
			findCells(minBlackRow+1, bottom, left, right); 
		}
	}
} // findLinesHarder

int overlapHorizontal(glyph_t *first, glyph_t *second) {
	if (!mayCombine) return 0;
	// fprintf(stderr, "in overlapHorizontal\n");
	if (!first || !second) return 0;
	// fprintf(stderr, "\tThere is both a first and second line\n");
	if ((first->right >= second->left && first->right <= second->right)
		||
	   (second->right >= first->left && second->right <= first->right)) {
	   // there is a horizontal overlap
		// fprintf(stderr, "\toverlap: [x=%d,x=%d] overlaps [x=%d,x=%d]\n",
		// 	first->left, first->right, second->left, second->right);
		int newHeight =
			MIN(first->top - second->bottom, second->top - first->bottom);
		if (newHeight > glyphHeight/2 || newHeight > maxGlyphHeight) {
			// too distant vertically
			// fprintf(stderr,
			// 	"\tat (%d,%d) too distant vertically (%d) to combine\n",
			// 	first->left, first->top,
			// 	MAX(first->top - second->bottom, second->top - first->bottom));
			return(false);
		}
		// fprintf(stderr, "\twill combine\n");
		return(true);
	} // horizontal overlap
	return 0;
} // overlapHorizontal

// returns boolean: true if the two lines either overlap vertically or
// combining them wouldn't largely increase the height of the taller.
int overlapVertical(lineHeaderList *first, lineHeaderList *second) {
	if (!first || !second) return 0;
	textLine *f = first->line;
	textLine *s = second->line;
	// if (f->top >= s->top && f->bottom <= s->bottom) return true;
	// if (s->top >= f->top && s->bottom <= f->bottom) return true;
	if (f->bottom >= s->top && f->bottom <= s->bottom) return true;
		// s within f
	if (s->bottom >= f->top && s->bottom <= f->bottom) return true;
		// f within s
	// int largerHeight = MAX(HEIGHT(first->line), HEIGHT(second->line));
	int combinedHeight = MAX(f->bottom, s->bottom) - MIN(f->top, s->top);
	if (combinedHeight < 2*glyphHeight) return true;
	// fprintf(stderr,
	// 	"combined height %d, glyphHeight %d\n",
	// 	combinedHeight, glyphHeight);
	return 0;
} // overlapVertical

void refillLine(lineHeaderList *aLineHeader) {
	// refill all tuples in the line
	glyph_t	*curGlyphPtr;
	for (curGlyphPtr = aLineHeader->line->glyphs->next; curGlyphPtr->next; 
				curGlyphPtr = curGlyphPtr->next) { // each actual glyph on line
		if (curGlyphPtr->tuple)
			refillTuple(curGlyphPtr, curGlyphPtr->tuple);
	}
} // refillLine

void adjustLineStats(textLine *aLine) {
	glyph_t *curGlyphPtr;
	int top=INFTY, bottom=-INFTY, left=INFTY, right=-INFTY;
	// fprintf(stderr, "adjusting line ");
	for (curGlyphPtr = aLine->glyphs->next; curGlyphPtr; 
			curGlyphPtr = curGlyphPtr->next) { // each glyph on line
		// fprintf(stderr, "(%d, %d) ", curGlyphPtr->left, curGlyphPtr->top);
		top = MIN(top, curGlyphPtr->top);
		left = MIN(left, curGlyphPtr->left);
		bottom = MAX(bottom, curGlyphPtr->bottom);
		right = MAX(right, curGlyphPtr->right);
	} // each glyph on line
	aLine->top = top;
	aLine->leftBorder = left;
	aLine->bottom = bottom;
	aLine->rightBorder = right;
	// fprintf(stderr, "\n");
} // adjustLineStats

int verticallyClose(glyph_t *aGlyph, textLine *aLine, textLine *nextLine) {
	// we might join aGlyph with aLine, if it is close, and nextLine isn't
	// better
	if (aLine->end == aLine->glyphs) return true; // aLine is empty
	// glyph_t *otherGlyph = aLine->end;
	if (aLine->bottom >= aGlyph->top && 
			aLine->bottom <= aGlyph->bottom)
		return true; // aGlyph within aLine
	if (aGlyph->bottom >= aLine->top &&
			aGlyph->bottom <= aLine->bottom)
		return true; // aLine within aGlyph
	/*
	if (MIN(abs(aGlyph->top - aLine->bottom),
			abs(aGlyph->bottom - aLine->top)) < glyphHeight/3) 
		return true;  // fairly close vertically
	*/
	// int newHeight = // result of combining
	// 	MAX(aGlyph->bottom, aLine->bottom) - MIN(aGlyph->top, aLine->top);
	int oldHeight = // bigger of the two things we want to combine
		MAX(HEIGHT(aLine), HEIGHT(aGlyph));
	int distance;
	if (aLine->bottom < aGlyph->top) { // glyph is below line
		distance = aGlyph->top - aLine->bottom;
	} else {
		distance = aLine->top - aGlyph->bottom;
	}
	// fprintf(stderr, "distance is %d\n", distance);
	if (distance < oldHeight / 5 /* || newHeight < oldHeight * 1.3 */) {
		if (nextLine) {
			if (nextLine->top - aGlyph->bottom < aGlyph->top - aLine->bottom) {
				// fprintf(stderr,
				// 	"glyph (%d,%d) is closer to next line (%d < %d)\n",
				// 	aGlyph->left, aGlyph->top,
				// 	nextLine->top - aGlyph->bottom,
				// 	aGlyph->top - aLine->bottom);
				return false;
			}
		}
		return true; // including aGlyph won't make line too big
	}
	// fprintf(stderr, "not close enough.  glyph at left,top (%d,%d) "
	// 	" would make a new height of %d; old height is %d.  Line is at y=[%d,%d]\n",
	// 	aGlyph->left, aGlyph->top, newHeight, oldHeight, aLine->top, aLine->bottom);
	return false;	
} // verticallyClose

lineHeaderList *newLineHeader() {
	lineHeaderList *answer = 
		(lineHeaderList *) malloc(sizeof(lineHeaderList));
	answer->line = (textLine *) malloc(sizeof(textLine));
	answer->line->glyphs = answer->line->end = // dummy at start
		(glyph_t *) calloc(sizeof(glyph_t), 1);
	answer->line->glyphs->next = NULL;
	answer->line->top = -1; // that is, undefined
	answer->line->bottom = -1; // that is, undefined
	answer->next = NULL;
	return answer;
} // newLineHeader

lineHeaderList *combineLines(lineHeaderList *curLine) { // combine with next.
	// we might end up with multiple lines (usually 1, sometimes 2, even 3).
	// We returned a pointer to the last of these lines; we relink curLine to
	// properly point to them all.
	// showGlyphs(curLine->line, "line 1 of 2");
	// showGlyphs(curLine->next->line, "line 2 of 2");
	// showLines();
	// the second line tends to be the shorter one; take its glyphs and
	// place them in the first line.
	textLine *firstLine = curLine->line;
	glyph_t *firstGlyph = firstLine->glyphs->next;
	textLine *secondLine = curLine->next->line;
	glyph_t *secondGlyph = secondLine->glyphs->next;
	lineHeaderList *newHeader = newLineHeader();
	while (firstGlyph || secondGlyph) { // treat one glyph
		// currentGlyph = leftmost input glyph
		glyph_t *currentGlyph =
			firstGlyph == NULL ? secondGlyph :
			secondGlyph == NULL ? firstGlyph :
			firstGlyph->left < secondGlyph->left ? firstGlyph :
			secondGlyph;
		if (currentGlyph == firstGlyph) {
			firstGlyph = firstGlyph->next;
		} else {
			secondGlyph = secondGlyph->next;
		}
		currentGlyph->next = NULL;
		// fprintf(stderr, "working on glyph x=[%d,%d] y=[%d,%d] \n",
		// 	currentGlyph->left, currentGlyph->right,
		// 	currentGlyph->top, currentGlyph->bottom
		// 	);
		// currentLine = proper output line to use
		lineHeaderList *currentLine = newHeader;
		while (!verticallyClose(currentGlyph, currentLine->line,
				currentLine->next ? currentLine->next->line : NULL)) {
			if (!currentLine->next) { // need to make another line
				// fprintf(stderr, "adding another line in combination\n");
				currentLine->next = newLineHeader();
			}
			currentLine = currentLine->next;
		}
		currentLine->line->end->next = currentGlyph;
		if (currentLine->line->top == -1) {
			currentLine->line->top = currentGlyph->top;
			currentLine->line->bottom = currentGlyph->bottom;
		} else {
			currentLine->line->top =
				MIN(currentGlyph->top, currentLine->line->top);
			currentLine->line->bottom =
				MAX(currentGlyph->bottom, currentLine->line->bottom);
		}
		currentLine->line->end = currentLine->line->end->next;
	} // treat glyph
	// showGlyphs(newHeader->line, "contents of first");
	// relink everything
	lineHeaderList *lastHeader; // end of new headers
	numLines -= 1;
	for (lastHeader = newHeader; lastHeader->next; lastHeader=lastHeader->next){
		numLines += 1;
	}
	lastHeader->next = curLine->next->next;
	free(curLine->line->glyphs); // rest of chain is already relinked
	free(curLine->line);
	curLine->line = newHeader->line; // we need to preserve curLine itself
	free(curLine->next->line->glyphs); // rest of chain is already relinked
	free(curLine->next->line); // no need to preserve curLine->next
	free(curLine->next);
	curLine->next = newHeader->next;
	// free(newHeader);
	lineHeaderList *ptr;
	for (ptr = curLine; ptr != lastHeader->next; ptr = ptr->next) {
		adjustLineStats(ptr->line);
	}
	// BUG:  The lines might be out of order.
	if (curLine->line == lastHeader->line) {
		free(newHeader);
		return(curLine);
	} else {
		free(newHeader);
		return(lastHeader);
	}
} // combineLines

lineHeaderList *combineLinesOld(lineHeaderList *curLine) { // combine with next.
	// return: (1) if completely combined, pointer to resulting line.
	// (2) otherwise, pointer to second line.
	// showGlyphs(curLine->line, "line 1 of 2");
	// showGlyphs(curLine->next->line, "line 2 of 2");
	// showLines();
	// the second line tends to be the shorter one; take its glyphs and
	// place them in the first line.
	lineHeaderList *removed = curLine->next;
	curLine->next = removed->next;
	glyph_t *thisGlyph, *nextGlyph;
	thisGlyph = removed->line->glyphs->next;
	// establish secondLine, which we may need for non-combinable glyphs
	lineHeaderList *secondLine =
		(lineHeaderList *) malloc(sizeof(lineHeaderList));
	secondLine->line = (textLine *) malloc(sizeof(textLine));
	secondLine->line->glyphs = secondLine->line->end =
		(glyph_t *) calloc(1, sizeof(glyph_t)); // dummy
	secondLine->line->glyphs->next = NULL;
	secondLine->next = NULL;
	while (thisGlyph) { // place thisGlyph in curLine
		// fprintf(stderr, "trying to place glyph at [%d,%d]\n",
		// 	thisGlyph->left, thisGlyph->right);
		nextGlyph = thisGlyph->next;
		thisGlyph->next = NULL;
		glyph_t *curGlyphPtr;
		for (curGlyphPtr = curLine->line->glyphs; curGlyphPtr->next; 
				curGlyphPtr = curGlyphPtr->next) { // each glyph on line
			// starting with the dummy at head of line!
			if (thisGlyph->right < curGlyphPtr->next->left) {
				// fprintf(stderr, "position is appropriate: [%d,%d]\n",
				// 	curGlyphPtr->next->left, curGlyphPtr->next->right);
				break; // we found the right position in line
			}
		} // each known glyph on line
		if (thisGlyph->top - curGlyphPtr->bottom > glyphHeight/2) {
			// not vertically close; place at end of second line
			// fprintf(stderr,
			// 	"glyph at (%d-%d,%d-%d) is pretty far from neighbor (%d-%d,%d-%d)\n",
			// 	thisGlyph->left, thisGlyph->right, thisGlyph->top, thisGlyph->bottom,
			// 	curGlyphPtr->left, curGlyphPtr->right, curGlyphPtr->top, curGlyphPtr->bottom
			// 	);
			thisGlyph->next = NULL;
			secondLine->line->end->next = thisGlyph;
			secondLine->line->end = thisGlyph;
			// adjustLineStats(secondLine->line); // for debugging
			// showGlyphs(secondLine->line, "second line has");
		} else {
			thisGlyph->next = curGlyphPtr->next;
			curGlyphPtr->next = thisGlyph; // whether we fell off the end or not
		}
		thisGlyph = nextGlyph;
	} // place thisGlyph in curLine
	free((void *) removed);
	// don't refillLine(curLine); we don't have any tuples yet.
	// showGlyphs(curLine->line, "after joining");
	// showLines();
	// adjust return values
	if (secondLine->line->glyphs->next == NULL) {
		// did not use second line
		fprintf(stderr, "did not use second line\n");
		free(secondLine->line->glyphs);
		free(secondLine->line);
		free(secondLine);
	 	adjustLineStats(curLine->line);
		numLines -= 1;
		return(curLine);
	} else { // used second line
		fprintf(stderr, "used second line\n");
		secondLine->next = curLine->next;
		curLine->next = secondLine;
		adjustLineStats(secondLine->line);
		adjustLineStats(curLine->line);
		if (secondLine->line->top < curLine->line->top) { // out of order
			fprintf(stderr, "need to reorder lines\n");
			textLine *tmp = curLine->line;
			curLine->line = secondLine->line;
			secondLine->line = tmp;
			adjustLineStats(secondLine->line);
			adjustLineStats(curLine->line);
		} // out of order
		// showLines(lineHeaders->next);
		return(secondLine);
	} // used second line
} // combineLinesOld

void copyGlyph(glyph_t *glyphPtr, lineHeaderList *theLineList) {
	lineHeaderList *curLineHeader;
	// Copy to the end of the line whose last glyph is close to glyphPtr.
	// If none, start a new line.
	glyph_t *newGlyph = (glyph_t *) calloc(1, sizeof(glyph_t));
	memcpy(newGlyph, glyphPtr, sizeof(glyph_t));
	newGlyph->next = NULL;
	for (curLineHeader = theLineList; curLineHeader->next;
			curLineHeader = curLineHeader->next) { // each known line
		textLine *tryLine = curLineHeader->next->line;
		glyph_t *prevGlyph = tryLine->end;
		// int xdistance = abs(newGlyph->left - prevGlyph->right);
		int ydistance = 
			MIN(abs(newGlyph->top - prevGlyph->top),
				abs(newGlyph->bottom - prevGlyph->bottom));
		// if (xdistance > glyphWidth * 10) continue;
		if (ydistance > 2*glyphHeight / 3) continue;
		// fprintf(stderr, "<%d>", ydistance);
		prevGlyph->next = newGlyph;
		tryLine->end = newGlyph;
		tryLine->top = MIN(tryLine->top, newGlyph->top);
		tryLine->leftBorder = MIN(tryLine->leftBorder, newGlyph->left);
		tryLine->bottom = MAX(tryLine->bottom, newGlyph->bottom);
		tryLine->rightBorder = MAX(tryLine->rightBorder, newGlyph->right);
		return;
	} // each line
	// none of the lines works.  Build a new one.
	// fprintf(stderr, "<!>");
	for (curLineHeader = theLineList; curLineHeader->next;
			curLineHeader = curLineHeader->next) { // each known line
		// sort by line top
		if (curLineHeader->next->line->top > newGlyph->top) break;
		// fprintf(stderr, "Not this line (%d <= %d)\n",
		// 	curLineHeader->line->top, newGlyph->top);
	}
	// place after curLineHeader.
	lineHeaderList *newLineHeader;
	newLineHeader = (lineHeaderList *) malloc(sizeof(lineHeaderList));
	newLineHeader->next = curLineHeader->next;
	curLineHeader->next = newLineHeader;
	newLineHeader->line = (textLine *) malloc(sizeof(textLine));
	newLineHeader->line->glyphs = calloc(1, sizeof(glyph_t)); // dummy
	newLineHeader->line->glyphs->next = newGlyph;
	newLineHeader->line->end = newGlyph;
	newLineHeader->line->top = newGlyph->top;
	newLineHeader->line->leftBorder = newGlyph->left;
	newLineHeader->line->bottom = newGlyph->bottom;
	newLineHeader->line->rightBorder = newGlyph->right;
} // copyGlyph

void splitLine(lineHeaderList *curLineHeader) {
	// curLineHeader is too tall; it may contain several lines.
	// return;
	glyph_t *glyphPtr;
	lineHeaderList *newLineHeader;
	newLineHeader = (lineHeaderList *) malloc(sizeof(lineHeaderList));
	newLineHeader->next = NULL;
	// fprintf(stderr, "glyphWidth = %d, glyphHeight = %d\n", glyphWidth,
	// 	glyphHeight);
	for (glyphPtr = curLineHeader->line->glyphs->next; glyphPtr;
				glyphPtr = glyphPtr->next){
		copyGlyph(glyphPtr, newLineHeader); // memory leak, but acceptable
	} // each glyph
	// fprintf(stderr, "\n");
	// showLines(newLineHeader);
	int count = 0;
	lineHeaderList *aLineHeader;
	// move to end of new lines in order to link properly
	for (aLineHeader = newLineHeader; aLineHeader->next;
			aLineHeader=aLineHeader->next) {
		count += 1;
		// fprintf(stderr,
		// 	"after splitting line, line %d with y range %d-%d\n",
		// 	count, aLineHeader->next->line->top,
		// 	aLineHeader->next->line->bottom);
		// showGlyphs(aLineHeader->next->line, "the glyphs");
	}
	aLineHeader->next = curLineHeader->next;
	curLineHeader->line = newLineHeader->next->line;
	curLineHeader->next = newLineHeader->next->next;
	refillLine(curLineHeader);
	if (curLineHeader->next) refillLine(curLineHeader->next);
} // splitLine

int leftBorder, rightBorder; // saved between calls to findLines

void computeBorders(int column) {
	// computes leftBorder, rightBorder (global)
	if (columns == 1) {
		leftBorder = 0;
		rightBorder = width-1;
		return;
	}
	int spread = width / (2*columns);
	int dividePoint, tryCount, bestCount;
	int lowMark, highMark;
	if (RTL && column < columns - 1) {
		rightBorder = leftBorder; // from previous iteration
		leftBorder = -1; // force computation
	} else if (!RTL && column > 0) {
		leftBorder = rightBorder; // from previous iteration
		rightBorder = -1; // force computation
	} else {
		leftBorder = -1; // force computation
		rightBorder = -1; // force computation
	}
	if (rightBorder == -1) { // compute highBorder
		highMark = (column+1) * width / columns; // expected divide
		if (countNonBlankCol(0, height-1, highMark) == 0) {
			// quick and easy, no need to search
			rightBorder = highMark;
		} else { // search for rightBorder
			rightBorder = 0; // ridiculous first guess
			bestCount = INFTY;
			for (dividePoint = MAX(0,highMark - spread);
					dividePoint < MIN(highMark + spread, width);
					dividePoint += 1) {
				tryCount = countNonBlankCol(0, height-1, dividePoint);
				if (tryCount <= bestCount &&
					abs(dividePoint - highMark) < abs(rightBorder-highMark)) {
					// closer to expected divide and better white space
					// fprintf(stderr, "R border: width %d has %d black\n",
					// 	dividePoint, tryCount);
					bestCount = tryCount;
					rightBorder = dividePoint;
				}
			} // each potential split point
		} // search for rightBorder
	} // compute rightBorder
	if (leftBorder == -1) { // compute leftBorder
		lowMark = column * width / columns; // expected divide
		if (countNonBlankCol(0, height-1, lowMark) == 0) {
			// quick and easy, no need to search
			leftBorder = lowMark;
		} else { // search for leftBorder
			leftBorder = width; // ridiculous first guess
			bestCount = INFTY;
			for (dividePoint = MAX(0,lowMark - spread);
					dividePoint < MIN(lowMark + spread, width);
					dividePoint += 1) {
				tryCount = countNonBlankCol(0, height-1, dividePoint);
				if (tryCount <= bestCount &&
						abs(dividePoint - lowMark) < abs(leftBorder-lowMark)) {
					// closer to expected divide and better white space
					// fprintf(stderr, "L border: width %d has %d black\n",
					// 	dividePoint, tryCount);
					bestCount = tryCount;
					leftBorder = dividePoint;
				}
			} // each potential split point
		} // search for leftBorder
	} // compute leftBorder
	// fprintf(stdout, "column is %d-%d out of %d\n", leftBorder,
	// 	rightBorder, width);
} // computeBorders

// Build a new glyph based on the first and second glyphs, which we join.
// Don't modify the two glyphs.
glyph_t *combineGlyphs(
	glyph_t *first, glyph_t *second)
{
	glyph_t *answer = calloc(sizeof(glyph_t), 1);
	int firstHeight = HEIGHT(first);
	int secondHeight = HEIGHT(second);
	answer->left = MIN(first->left, second->left);
	answer->right = MAX(first->right, second->right);
	answer->top = MIN(first->top, second->top);
	answer->bottom = MAX(first->bottom, second->bottom);
	answer->lineHeight = first->lineHeight;
	answer->next = second->next;
	int newHeight = HEIGHT(answer);
	if (first->leftPath || first->rightPath
			|| second->leftPath || second->rightPath) {
		// need to calculate new right and left paths
		answer->leftPath = (int *) calloc(newHeight, sizeof(int));
		answer->rightPath = (int *) calloc(newHeight, sizeof(int));
		int row;
		int *leftPtr = answer->leftPath;
		int *rightPtr = answer->rightPath;
		for (row = answer->top; row < answer->bottom; row += 1) {
			int firstOffset = row - first->top;
			int secondOffset = row - second->top;
			int leftValue = INFTY; // starting value
			if (first->leftPath && firstOffset >= 0
					&& firstOffset < firstHeight) {
				leftValue = MIN(leftValue, first->leftPath[firstOffset]);
			}
			if (second->leftPath && secondOffset >= 0 &&
					secondOffset < secondHeight) {
				leftValue = MIN(leftValue, second->leftPath[secondOffset]);
			}
			if (leftValue == INFTY) { // must be no paths
				leftValue = MIN(first->left, second->left);
			}
			*leftPtr = leftValue;
			leftPtr += 1;
			int rightValue = 0; // starting value
			if (first->rightPath && firstOffset >= 0
					&& firstOffset < firstHeight) {
				rightValue = MAX(rightValue, first->rightPath[firstOffset]);
			}
			if (second->rightPath && secondOffset >= 0
					&& secondOffset < secondHeight) {
				rightValue = MAX(rightValue, second->rightPath[secondOffset]);
			}
			if (rightValue == 0) { // must be no paths
				rightValue = MAX(first->right, second->right);
			}
			*rightPtr = rightValue;
			rightPtr += 1;
		} // each row
	} // calculate new left and right paths
	answer->tuple = 0;
	answer->distance = -1;
	calculateDistance(answer);
	return(answer);
} // combineGlyphs

// copy the fields of new onto old, freeing as necessary in old,
// then free new.
void overWrite(glyph_t *old, glyph_t *new) {
	if (old->leftPath) free(old->leftPath);
	if (old->rightPath) free(old->rightPath);
	if (old->tuple) free(old->tuple);
	memcpy(old, new, sizeof(glyph_t));
	free(new);
} // overWrite

// discover the lines for a given column out of all the columns.
// columns == 1: column is 0
// else: (RTL: columns-1, columns-2, ..., 0) or (LTR: 0 ... columns-1)
int findLines(int column) { // returns Boolean for success
	// fprintf(stderr,
	// 	"Direction %s, column %d / %d\n", RTL ? "RTL" : "LTR", column, columns);
	// statistics
	leftMargin = INFTY; // over-estimate
	rightMargin = 0; // under-estimate
	// allocation
	lineHeaders = (lineHeaderList *) calloc(1, sizeof(lineHeaderList));
	// analysis of the image
	computeBorders(column); // computes leftBorder, rightBorder (global)
	if (useFlood) {
		floodFindGlyphs(column);
	} else {
		findCells(0, height-1, leftBorder, rightBorder);
	}
	// fprintf(stderr, "average width %d, height %d\n", glyphWidth, glyphHeight);
	// count lines
	numLines = 0;
	lineHeaderList *curLine;
	for (curLine = lineHeaders->next; curLine; curLine = curLine->next) {
		numLines += 1;
	}
	fprintf(stderr, "There are %d lines in column %d\n", numLines, column);
	int numGlyphs = 0;
	int lineNumber = 0;
	int totalWidth = 0;
	int totalHeight = 0; // sum of individual lines
	for (curLine = lineHeaders->next; curLine; curLine = curLine->next) {
		// combine glyphs that share horizontal space
		// showGlyphs(curLine->line, "contents of line before");
		lineHeaderList *thisLine = curLine;
		while (overlapVertical(thisLine, thisLine->next)) {
			// lineHeaderList *orig = thisLine;
			// lineHeaderList *next = thisLine->next;
			// fprintf(stderr, "joining line %d, y=[%d,%d] with next, top %d\n",
			// 	lineNumber, thisLine->line->top, 
			// 	thisLine->line->bottom,
			// 	thisLine->next->line->top);
			// showGlyphs(thisLine->line, "first line before joining");
			// showGlyphs(thisLine->next->line, "second line before joining");
			thisLine = combineLines(thisLine); // may return second line
			if (thisLine == curLine) {
				// showGlyphs(thisLine->line, "returned first line");
				// fprintf(stderr, "first line is at y=[%d,%d]\n",
				// 	thisLine->line->top, thisLine->line->bottom);
			} else if (thisLine == curLine->next) {
				// showGlyphs(curLine->line, "first of two lines");
				// showGlyphs(thisLine->line, "second of two lines");
			} else {
				// fprintf(stderr, "\treturned new line\n");
			}	
			// fprintf(stderr, "\twe now have %d lines\n", numLines);
		} // while overlapVertical 
		// compute avgHeight so we can split tall lines later
		if (thisLine->next) {
			// fprintf(stderr,
			// 	"adjacent lines don't overlap: y=[%d,%d], y=[%d,%d]\n",
			// 	thisLine->line->top,
			// 	thisLine->line->bottom,
			// 	thisLine->next->line->top,
			// 	thisLine->next->line->bottom);
		};
		int lineHeight = HEIGHT(curLine->line);
		totalHeight += lineHeight;
		// fix the lineHeight field of all glyphs on this line, combine horiz.
		glyph_t *glyphPtr;
		int changedLine = true;
		while (changedLine) { // once we combine glyphs we need to do it again.
			changedLine = false;
			for (glyphPtr = curLine->line->glyphs->next; glyphPtr;
					glyphPtr = glyphPtr->next){
				glyphPtr->lineHeight = lineHeight;
				glyph_t *next = glyphPtr->next;
				while (overlapHorizontal(glyphPtr, next)) { // overlap; combine
					if (next->right - glyphPtr->left > maxGlyphWidth) {
						// don't combine even though overlap
						break;
					}
					glyph_t *newGlyph = combineGlyphs(glyphPtr, next);
					if (newGlyph->bottom - newGlyph->top > maxGlyphHeight) {
						// don't combine; it's too high
						freeGlyph(newGlyph);
						break;
					}
					calculateDistance(glyphPtr);
					calculateDistance(next);
					// fprintf(stderr, "%f vs %f, %f\n",
					// 	newGlyph->distance,
					// 	glyphPtr->distance, next->distance);
					if (!alwaysCombine && newGlyph->distance >
							MAX(glyphPtr->distance, next->distance)) {
						// don't combine; it's not very good
						// fprintf(stderr,
						// 	"not combining at [%d, %d]\n",
						// 	newGlyph->left, newGlyph->top);
						freeGlyph(newGlyph);
						break;
					}
					// fprintf(stderr, "combining at [%d, %d]\n",
					//		newGlyph->left, newGlyph->top);
					freeGlyph(next);
					overWrite(glyphPtr, newGlyph);
					next = glyphPtr->next;
					changedLine = true;
				} // overlap horizontal
				numGlyphs += 1;
				totalWidth += glyphPtr->right - glyphPtr->left;
				// fprintf(stderr, "[%d]", glyphPtr->right - glyphPtr->left);
			} // each glyph: give it the right height
		} // while line changed
		lineNumber += 1;
	} // each line
	// see if there any very high lines; maybe we should split them
	int lineIndex;
	float avgHeight = ((float) totalHeight)/lineNumber;
	lineIndex = 0;
	for (curLine = lineHeaders->next; curLine; curLine = curLine->next) {
		lineIndex += 1;
		if (HEIGHT(curLine->line) > 1.5*avgHeight) {
			// fprintf(stderr, "Line %d looks too high: %d (avg is %d).\n",
			// 	lineIndex, HEIGHT(curLine->line), (int) avgHeight);
			splitLine(curLine);
		} // too high
		glyph_t *glyphPtr;
		for (glyphPtr = curLine->line->glyphs->next; glyphPtr;
				glyphPtr = glyphPtr->next){
			glyphPtr->lineHeight = HEIGHT(curLine->line);
		}
	} // each line
	if (numGlyphs == 0) {
		fprintf(stderr, "No letters are found in this image.\n");
		return(false);
	}
	glyphWidth = totalWidth / numGlyphs;
	// fprintf(stderr, "total of %d glyphs; average width %d\n", numGlyphs,
	// 	glyphWidth);
	return(true);
} // findLines

// see if we can find a blank line somewhere near the given row, looking
// not too far afield either up or down.
int scanAbout(int row, int left, int right) {
	int top, bottom;
	int limit = row - averageLineHeight;
	for (top = row; top > limit; top -= 1) { 
		if (isBlankRow(top, left, right)) break;
	} 
	limit = row + averageLineHeight;
	for (bottom = row; bottom < limit; bottom += 1) { 
		if (isBlankRow(bottom, left, right)) break;
	} 
	if (bottom - row < row - top) return(bottom); // going down is closer
	if (bottom - row > row - top) return(top); // going up is closer
	if (bottom == limit) return(row); // no luck up or down
	return(top); // equally good; go up.
} // scanAbout

glyph_t *glyphAtX(textLine *theLine, int row, int col, int verbose) {
	glyph_t *theGlyph;
	for (theGlyph = theLine->glyphs->next; theGlyph;
			theGlyph = theGlyph->next) {
		// fprintf(stdout, "char ");
		// fprintf(stderr, "a glyph at %d-%d\n", theGlyph->left, theGlyph->right);
		if (theGlyph->left <= col && col < theGlyph->right && 
				theGlyph->top <= row && row < theGlyph->bottom) {
			return(theGlyph);
		}
		// we cannot be sure the glyphs are sorted by col, so keep looking
	} // each glyph
	if (verbose) {
		fprintf(stderr, "on line with y in [%d, %d]\n",
			theLine->top, theLine->bottom);
		fprintf(stderr, "Got to end of line\n");
	}
	return(NULL);
} // glyphAtX

// return the glyph at the given row, column; NULL if none
glyph_t *glyphAtXY(int col, int row, int verbose) {
	// fprintf(stderr, "looking for x=%d, y=%d\n", col, row);
	lineHeaderList *curLine;
	glyph_t *answer = NULL;
	for (curLine = lineHeaders->next; curLine; curLine = curLine->next) {
		if (verbose) {
			fprintf(stderr, "y range [%d,%d]\n", curLine->line->top,
				curLine->line->bottom);
		}
		if (curLine->line->top <= row && row < curLine->line->bottom) {
			answer = glyphAtX(curLine->line, row, col, verbose);
			if (answer) return(answer);
		}
		// if (row < curLine->line->top) return(NULL); // too far
	} // each line
	// fprintf(stdout, "most likely line %d, y value %d-%d\n", lineIndex,
	// 	curLine->line->top, curLine->line->bottom);
	if (verbose) {
		fprintf(stderr, "past the last line on page\n");
	}
	return(NULL);
} // glyphAtXY

void narrowGlyph(glyph_t *glyph) {
	// adjust the borders of glyph to account for any leftPath and rightPath
		// or any splitting
	// fprintf(stderr, "Narrowing glyph [%d,%d]-[%d,%d]: ",
	// 	glyph->left, glyph->top, glyph->right, glyph->bottom);
	// move the top down
	int newTop;
	register int top = glyph->top;
	for (newTop = top; newTop < glyph->bottom; newTop += 1) {
		int left, right;
		left = (glyph->leftPath)
			? glyph->leftPath[newTop - top]
			: glyph->left;
		right = (glyph->rightPath)
			? glyph->rightPath[newTop - top] + 1
			: glyph->right;
		if (right - left < 1) {
			// fprintf(stderr, " BUG right overlaps left?\n");
			continue;
		}
		if (!isBlankRow(newTop, left, right)) break;
	}
	int lowering = newTop - top;
	if (lowering) { // must readjust leftPath and rightPath
		// fprintf(stderr, " lowering by %d", lowering);
		if (glyph->leftPath) {
			memmove(glyph->leftPath, &glyph->leftPath[lowering],
				(HEIGHT(glyph) - lowering) * sizeof(int));
		} // adjust leftPath
		if (glyph->rightPath) {
			memmove(glyph->rightPath, &glyph->rightPath[lowering],
				(HEIGHT(glyph) - lowering)*sizeof(int));
		} // adjust leftPath
		glyph->top = newTop;
		top = glyph->top; // for efficiency
	} // must readjust
	// move the bottom up
	int newBottom;
	for (newBottom = glyph->bottom-1; newBottom > top; newBottom -= 1) {
		int left, right;
		left = (glyph->leftPath)
			? glyph->leftPath[newBottom - top]
			: glyph->left;
		right = (glyph->rightPath)
			? glyph->rightPath[newBottom - top]
			: glyph->right;
		if (!isBlankRow(newBottom, left, right)) break;
	}
	glyph->bottom = newBottom+1;
	// adjust the left only if necessary
	if (glyph->leftPath) {
		// fprintf(stderr, " [leftPath]");
		int newLeft = glyph->right; // pseudo-data
		int row;
		for (row = top; row < glyph->bottom; row += 1) {
			int left;
			for (left = MAX(glyph->left, glyph->leftPath[row - top]);
					left < newLeft;
					left += 1) {
				if (ISFILLED(row,left)) {
					newLeft = left;
					break;
				}
			} // each left
		} // each row
		glyph->left = newLeft;
	} // there is a leftPath
	// adjust the right only if necessary
	if (glyph->rightPath) {
		// fprintf(stderr, " [rightPath]");
		int newRight = glyph->left; // pseudo-data
		int row;
		for (row = top; row < glyph->bottom; row += 1) {
			int right;
			for (right = glyph->rightPath[row - top]; right > newRight;
					right -= 1) {
				if (ISFILLED(row,right)) {
					newRight = right;
					break;
				}
			} // each right
		} // each row
		glyph->right = newRight+1;
	} // there is a rightPath
	// fprintf(stderr, " to  [%d,%d]-[%d,%d]\n",
	// 	glyph->left, top, glyph->right, glyph->bottom);
} // narrowGlyph

float evaluatePathSplit(glyph_t *glyphPtr, glyph_t *first, int *splitPath) {
// return the distance2 for the first part of a split of glyphPtr using
// splitPath
	*first = *glyphPtr;
	first->leftPath = copyPath(first->leftPath, HEIGHT(first));
	first->rightPath = copyPath(splitPath, HEIGHT(first));
	// caution: don't modify paths/borders until first, second
		// are properly formed!
	narrowGlyph(first); // modifies paths/borders
	first->tuple = newTuple();
	fillTuple(first, first->tuple);
	return ocrDistance2(first->tuple);
} // evaluatePathSplit

void splitWideGlyphs() {
	// look at tmp/sutskever/nybc202166_0011.tif, yud-resh cluster.
	// any glyph that looks very wide is worth splitting, based on classifier.
	lineHeaderList *curLine;
	for (curLine = lineHeaders->next; curLine; curLine = curLine->next) {
		glyph_t *glyphPtr;
		for (glyphPtr = curLine->line->glyphs->next; glyphPtr;
			glyphPtr = glyphPtr->next) {
			// fprintf(stderr, "looking at glyph starting at %d, width %d\n",
			// 	glyphPtr->left, glyphPtr->right - glyphPtr->left + 1);
			if (glyphPtr->right - glyphPtr->left > splittable*glyphWidth &&
					ocrDistance2(glyphPtr->tuple) > goodMatch2) {
				// fprintf(stderr,
				//	"trying to split; glyphWidth is %d; my width is %d; "
				//	"splittable %g\n",
				//	glyphWidth, glyphPtr->right - glyphPtr->left,
				//	splittable);
				glyph_t *first, *second;
				int *splitPath;
				if (!useFlood) { // not flooding; try path split 
					first = (glyph_t *) calloc(1, sizeof(glyph_t));
					splitPath = tryPathSeparation(glyphPtr);
					if (splitPath) { // try path split
						if (evaluatePathSplit(glyphPtr, first, splitPath) <
								minMatch) {
							// successful path split
							fprintf(stderr,
								"path splitting at [%d,%d]\n",
								first->left, first->top);
							finishPathSplit(glyphPtr, first, splitPath);
							continue; // go on to next glyph
						} // successful path split
						else { // unsuccessful; clean up
							freeGlyph(first);
						} // unsuccessful; clean up
						free(splitPath); // we always copy it
					} // try path split
				} // not flooding; try path split
				int splitPoint, bestSplitPoint;
				float bestCost = 2*minMatch;
				float firstOCR2 = 2*minMatch;
				for (splitPoint = glyphPtr->left + minGlyphWidth;
						splitPoint < glyphPtr->right - minGlyphWidth;
						splitPoint += 1) { // try this splitPoint
					first = (glyph_t *) calloc(1, sizeof(glyph_t));
					*first = *glyphPtr;
					first->leftPath = copyPath(first->leftPath, HEIGHT(first));
					first->rightPath = 0; // don't want any original rightpath
					first->right = splitPoint-(minGlyphWidth/SPLITGAP);
						// - to avoid noise just at the split
					second = (glyph_t *) calloc(1, sizeof(glyph_t));
					*second = *glyphPtr;
					second->left = splitPoint+(minGlyphWidth/SPLITGAP);
						// + to avoid noise just at the split
					second->leftPath =
						copyPath(second->leftPath, HEIGHT(second));
					second->rightPath =
						copyPath(second->rightPath, HEIGHT(second));
					// adjust bounds of first and second
					narrowGlyph(first);
					narrowGlyph(second);
					first->tuple = newTuple();
					fillTuple(first, first->tuple);
					second->tuple = newTuple();
					fillTuple(second, second->tuple);
					float theDistance = // adding squares, but finding max
						ocrDistance2(first->tuple) +
						ocrDistance2(second->tuple);
					// float theDistance = ocrDistance2(first->tuple);
					if (theDistance <= bestCost) {
						bestCost = theDistance;
						firstOCR2 = ocrDistance2(first->tuple);
						// fprintf(stderr, "\tsplitPoint %d firstOCR2 %f theDistance %f\n",
						// 	splitPoint, firstOCR2, theDistance);
						bestSplitPoint = splitPoint;
					}
					freeGlyph(first); // easier than just freeing parts
					freeGlyph(second);
				} // each splitpoint
				if (firstOCR2 < minMatch2) { // looks like success 
					// fprintf(stderr,
					// 	"vertical splitting [%d,%d] at %d; %f < %f\n",
					// 	glyphPtr->left, glyphPtr->top,
					// 	bestSplitPoint, firstOCR2, minMatch2);
					// glyphPtr will be the first glyph
					second = (glyph_t *) calloc(1, sizeof(glyph_t));
					*second = *glyphPtr;
					second->left = bestSplitPoint+(minGlyphWidth/SPLITGAP);
					second->leftPath = 0;
					second->rightPath = glyphPtr->rightPath;
					second->tuple = 0;
					glyphPtr->right = bestSplitPoint-(minGlyphWidth/SPLITGAP);
					glyphPtr->rightPath = 0;
					// adjust bounds of glyphPtr and second
					narrowGlyph(glyphPtr);
					narrowGlyph(second);
					fillTuple(glyphPtr, glyphPtr->tuple);
					second->tuple = newTuple();
					fillTuple(second, second->tuple);
					// previous glyph points to *glyphPtr, so we retain it.
					second->next = glyphPtr->next;
					glyphPtr->next = second;
					continue; // go on to next glyph
				} // success at vertical split
				// try slanted split
				splitPath = buildSlantedPath(glyphPtr, 
					glyphPtr->left + HEIGHT(glyphPtr)/slant +
					minGlyphWidth);
				// fprintf(stderr, "point 1: glyph [%d,%d]\n",
				// 	glyphPtr->left, glyphPtr->top);
				bestCost = 2*minMatch;
				for ( splitPoint = splitPath[0];
						splitPoint < glyphPtr->right - minGlyphWidth;
						splitPoint += 1) { // try this splitPoint
					// fprintf(stderr, "glyph [%d,%d], trying split at %d\n",
					// 	glyphPtr->left, glyphPtr->top, splitPoint);
					first = (glyph_t *) calloc(1, sizeof(glyph_t));
					float firstCost =
						evaluatePathSplit(glyphPtr, first, splitPath);
					// fprintf(stderr, "[x%d,h%d:%3.2f]", splitPoint,
					// 	HEIGHT(first),firstCost);
					if(firstCost < minMatch) { // successful split point
						if (firstCost < bestCost) { // best so far
							if (0)
								fprintf(stderr, "\tbetter: %3.2f at %d\n",
									firstCost, splitPoint);
							bestCost = firstCost;
							bestSplitPoint = splitPoint;
						} else {
							if (0)
							fprintf(stderr, "\tworse: %3.2f at %d\n",
								firstCost, splitPoint);
						}
					} // successful split point
					else {
						// fprintf(stderr, "\tunacceptable: %3.2f at %d\n",
						// 	firstCost, splitPoint);
					}
					// prepare for next try
					freeGlyph(first);
					shiftRight(splitPath, HEIGHT(glyphPtr));
				} // each splitpoint
				free(splitPath);
				if (bestCost < minMatch) { // found a good slant split
					if (0) fprintf(stderr,
						"slant splitting [%d,%d] at %d gives distance %3.2f\n",
						glyphPtr->left, glyphPtr->top, bestSplitPoint,
						bestCost);
					splitPath = buildSlantedPath(glyphPtr, bestSplitPoint);
					first = (glyph_t *) calloc(1, sizeof(glyph_t));
					(void) evaluatePathSplit(glyphPtr, first, splitPath);
					finishPathSplit(glyphPtr, first, splitPath);
					free(splitPath);
				} // found a good slant split
				else {
					if (0)
						fprintf(stderr, "failed to split [%d,%d]\n",
							glyphPtr->left, glyphPtr->top);
				}
			} // might want to split
		} // each glyph on the line	
	} // each line
} // splitWideGlyphs

void freeGlyph(glyph_t *glyphPtr) {
	if (!glyphPtr) return;
	if (glyphPtr->leftPath) free(glyphPtr->leftPath);
	if (glyphPtr->rightPath) free(glyphPtr->rightPath);
	if (glyphPtr->tuple) free(glyphPtr->tuple);
	free(glyphPtr);
} // freeGlyph

int *adjustPath(int *oldPath, int oldHeight, int stretchAbove,
		int stretchBelow) {
	// The oldPath was for a glyph that has been vertically extended, so the
	// new path must be longer.  We simply extend the first/last elements of
	// oldPath into a newly allocated array.  We do not deallocate oldPath.  
	int *newPath = (int *)
		malloc((oldHeight + stretchAbove + stretchBelow)*sizeof(int));
	memcpy(newPath+stretchAbove, oldPath, oldHeight*sizeof(int));
	int index;
	for (index = 0; index < stretchAbove; index += 1) {
		newPath[index] = oldPath[0];
	}
	for (index = 0; index < stretchBelow; index += 1) {
		newPath[stretchAbove+oldHeight+index] = oldPath[oldHeight-1];
	}
	return(newPath);
} // adjustPath

void narrowGlyphs() {
	// if there are two adjacent narrow unrecognizable glyphs, consider
	// combining them.
	lineHeaderList *curLine;
	for (curLine = lineHeaders->next; curLine; curLine = curLine->next) {
		glyph_t *glyphPtr, *prev;
		prev = curLine->line->glyphs;
		for (glyphPtr = prev->next;
				glyphPtr && glyphPtr->next;
				prev = glyphPtr, glyphPtr = glyphPtr->next) {
			if (glyphPtr->next->left - glyphPtr->right > minGlyphWidth) {
				continue; // no sense combining distant glyphs
			}
			float myOCR2 = ocrDistance2(glyphPtr->tuple);
			float nextOCR2 = ocrDistance2(glyphPtr->next->tuple);
			// fprintf(stderr,
			// 	"looking at glyph starting at %d, width %d, ocr %0.2f,"
			// 	"next ocr %0.2f\n",
			// 	glyphPtr->left, glyphPtr->right - glyphPtr->left,
			// 	myOCR, nextOCR);
			if (((myOCR2 > goodMatch2) || (nextOCR2 > goodMatch2))) {
				// consider combining
				glyph_t *combinedPtr = combineGlyphs(glyphPtr, glyphPtr->next);
				float combineOCR2 = ocrDistance2(combinedPtr->tuple);
				// fprintf(stderr, "\tcombineOCR %0.2f\n", combineOCR);
				if (combineOCR2 < goodMatch2) { // combine
					// fprintf(stderr, "\tYes!\n");
					freeGlyph(glyphPtr->next);
					overWrite(glyphPtr, combinedPtr);
				} else { // won't combine two
					if (glyphPtr->next->next &&
						glyphPtr->next->next->left - glyphPtr->next->right <
						minGlyphWidth
					) { // maybe combine three?
						float nextNextOCR2 =
							ocrDistance2(glyphPtr->next->next->tuple);
						// fprintf(stderr, "%d: %0.2f %0.2f %0.2f \n",
						// 	glyphPtr->left, myOCR, nextOCR, nextNextOCR);
						if (nextNextOCR2 > goodMatch2) { // try three combine
							glyph_t *ccombinedPtr = combineGlyphs(combinedPtr,
								glyphPtr->next->next);
							float ccombineOCR2 =
								ocrDistance2(ccombinedPtr->tuple);
							// fprintf(stderr, "\t%0.2f\n", ccombineOCR);
							if (ccombineOCR2 < goodMatch2) { // yes!
								freeGlyph(glyphPtr->next->next);
								freeGlyph(glyphPtr->next);
								freeGlyph(combinedPtr);
								overWrite(glyphPtr, ccombinedPtr);
							} else { // no luck with three
								freeGlyph(ccombinedPtr);
							}
						} else { // don't try for three combine
							freeGlyph(combinedPtr);
						}
					} else { // there is no third to combine
						freeGlyph(combinedPtr);
					}
				} // don't combine two
			} // consider combining
		} // each glyph on the line	
	} // each line
} // narrowGlyphs

void freeLines() {
	lineHeaderList *curLine, *prevLine;
	curLine = NULL;
	prevLine = curLine;
	for (curLine = lineHeaders->next; curLine; curLine = curLine->next) {
		glyph_t *glyphPtr, *prevPtr;
		prevPtr = curLine->line->glyphs;
		int first = 1;
		for (glyphPtr = curLine->line->glyphs->next; glyphPtr;
				glyphPtr = glyphPtr->next) {
			if (0) { // old code
				if (!first) {
					freeGlyph(prevPtr);
					free(prevPtr->tuple);
					// if (prevPtr->leftPath) free(prevPtr->leftPath); // BUG
					if (prevPtr->rightPath) free(prevPtr->rightPath);
				}
				free(prevPtr);
			} // old code
			else { // new code
				freeGlyph(prevPtr);
			}
			prevPtr = glyphPtr;
			first = 0;
		} // each glyph
		if (prevPtr) free(prevPtr);
		// free(prevLine->line);
		free(prevLine); // has no effect if prevLine == NULL
		prevLine = curLine;
	} // each line
	if (prevLine) {
		// free(prevLine->line);
		free(prevLine);
	}
} // freeLines

// macros for building a path between two glyphs
#define DOWN \
			if (ISBLANK(yIndex+1, xIndex)) { \
				yIndex += 1; \
				positions[yIndex-top] = xIndex; \
				continue; \
			}
#define DOWNLEFT \
			if (xIndex > glyphPtr->left && /* not at left edge */ \
				ISBLANK(yIndex, xIndex-1) && /* left is blank */ \
				ISBLANK(yIndex+1, xIndex-1) /* down-left is blank */ \
			) { /* left-down works */ \
				yIndex += 1; \
				positions[yIndex-top] = xIndex-1; \
				continue; /* keep going down */ \
			}
#define DOWNRIGHT \
			if (xIndex < glyphPtr->right-1 && /* not at right edge */ \
				ISBLANK(yIndex, xIndex+1) && /* right is blank */ \
				ISBLANK(yIndex+1, xIndex+1) /* down-right is blank */ \
			) { /* right-down works */ \
				yIndex += 1; \
				positions[yIndex-top] = xIndex+1; \
				continue; /* keep going down */ \
			}
#define DOWNLEFTLEFT \
			if (xIndex > glyphPtr->left+1 && /* not near left edge */ \
				ISBLANK(yIndex, xIndex-1) && /* left is blank */ \
				ISBLANK(yIndex, xIndex-2) && /* double-left is blank */ \
				ISBLANK(yIndex+1, xIndex-2) /* down double-left is blank */ \
			) { /* down double-left works */ \
				/* fprintf(stderr, "[ddl]"); */ \
				positions[yIndex-top] = xIndex-1; \
				yIndex += 1; \
				positions[yIndex-top] = xIndex-2; \
				continue; \
			}
#define DOWNRIGHTRIGHT \
			if (xIndex < glyphPtr->right-2 && /* not near right edge */ \
				ISBLANK(yIndex, xIndex+1) && /* right is blank */ \
				ISBLANK(yIndex, xIndex+2) && /* double-right is blank */ \
				ISBLANK(yIndex+1, xIndex+2) /* down double-right is blank */ \
			) { /* down double-right works */ \
				/* fprintf(stderr, "[ddr]"); */ \
				positions[yIndex-top] = xIndex+1; \
				yIndex += 1; \
				positions[yIndex-top] = xIndex+2; \
				continue; \
			}

int *tryPathSeparation(glyph_t *glyphPtr) {
// try to separate the glyph with a vertical-like path; return path on success
	// fprintf(stderr, "Trying to split glyph at (%d,%d)\n", glyphPtr->left,
	// 	glyphPtr->top);
	int height = HEIGHT(glyphPtr);
	int *positions = (int *) malloc(sizeof(int)*height);
	int xIndex, yIndex, xStartIndex;
	register int top = glyphPtr->top;
	yIndex = top;
	// first try with forward italic correction
	// find a starting white pixel at the top left
	for (xStartIndex = glyphPtr->left;
			xStartIndex < glyphPtr->right;
			xStartIndex += 1) { // start path at top, xStartIndex
		xIndex = xStartIndex;
		positions[0] = xIndex;
		if (ISFILLED(yIndex, xIndex)) continue; // bad starting place
		// fprintf(stderr, "starting at x=%d\n", xIndex);
		yIndex = top;
		// NOTE: we try to make progress at every step; no real backtracking
		while (yIndex != glyphPtr->bottom-1) { // still working
			xIndex = positions[yIndex-top];
			if ((xStartIndex - xIndex)*slant < (yIndex-top)) {
				// prefer left
				DOWNLEFT;
				DOWN;
				DOWNRIGHT;
				DOWNLEFTLEFT;
				DOWNRIGHTRIGHT;
			} else { // prefer straight down
				DOWN;
				DOWNLEFT;
				DOWNRIGHT;
				DOWNLEFTLEFT;
				DOWNRIGHTRIGHT;
			}
			// fprintf(stderr, "failed to find simple path starting at x=%d\n",
			//	xStartIndex);
			break;
		} // still working
		if (yIndex != glyphPtr->bottom-1) continue; // stopped before finishing
		// fprintf(stderr, "found first path for [%d,%d]; overall slant %3.2f\n", 
		// 	glyphPtr->left, top,
		// 	(glyphPtr->bottom - top + 0.0) / (xIndex - xStartIndex));
		return(positions);
	} // start path at top, xStartIndex
	// now try with a backward slant, like "\"
	// find a starting white pixel at the top left
	for (xStartIndex = glyphPtr->left;
			xStartIndex < glyphPtr->right;
			xStartIndex += 1) { // start path at top, xStartIndex
		xIndex = xStartIndex;
		positions[0] = xIndex;
		if (ISFILLED(yIndex, xIndex)) continue; // bad starting place
		// fprintf(stderr, "starting at x=%d\n", xIndex);
		yIndex = top;
		// NOTE: we try to make progress at every step; no real backtracking
		while (yIndex != glyphPtr->bottom-1) { // still working
			xIndex = positions[yIndex-top];
			if ((xIndex - xStartIndex)*slant < (yIndex-top)) {
				// prefer right
				DOWNRIGHT;
				DOWN;
				DOWNLEFT;
				DOWNRIGHTRIGHT;
				DOWNLEFTLEFT;
			} else { // prefer straight down
				DOWN;
				DOWNRIGHT;
				DOWNLEFT;
				DOWNRIGHTRIGHT;
				DOWNLEFTLEFT;
			}
			// fprintf(stderr, "failed to find simple path starting at x=%d\n",
			//	xStartIndex);
			break;
		} // still working
		if (yIndex != glyphPtr->bottom-1) continue; // stopped before finishing
		// fprintf(stderr, "found second path for [%d,%d]; overall slant %3.2f\n", 
		// 	glyphPtr->left, top,
		// 	(glyphPtr->bottom - top + 0.0) / (xIndex - xStartIndex));
		return(positions);
	} // start path at top, xStartIndex
	// fprintf(stderr, "failed to find simple path starting anywhere\n");
	free(positions);
	return(0);
} // tryPathSeparation

int *tryPathSeparationOld(glyph_t *glyphPtr) { // working, to be improved
// try to separate the glyph with a vertical-like path; return path on success
	// fprintf(stderr, "Trying to split glyph at (%d,%d)\n", glyphPtr->left,
	// 	glyphPtr->top);
	int height = HEIGHT(glyphPtr);
	int *positions = (int *) malloc(sizeof(int)*height);
	int xIndex, yIndex, xStartIndex;
	yIndex = glyphPtr->top;
	// find a starting white pixel at the top left
	for (xStartIndex = glyphPtr->left;
			xStartIndex < glyphPtr->right;
			xStartIndex += 1) { // start path at top, xStartIndex
		xIndex = xStartIndex;
		positions[0] = xIndex;
		if (ISFILLED(yIndex, xIndex)) continue; // bad starting place
		// fprintf(stderr, "starting at x=%d\n", xIndex);
		yIndex = glyphPtr->top;
		while (yIndex != glyphPtr->bottom-1) { // still working
			// try straight down
			xIndex = positions[yIndex-glyphPtr->top];
			if (ISBLANK(yIndex+1, xIndex)) { // down works
				yIndex += 1;
				positions[yIndex-glyphPtr->top] = xIndex;
				continue; // keep going down
			}
			// try down-left; requires that left also be blank
			else if (xIndex > glyphPtr->left && // not at left edge
				ISBLANK(yIndex, xIndex-1) && // left is blank
				ISBLANK(yIndex+1, xIndex-1) // down-left is blank
			) { // left-down works
				yIndex += 1;
				positions[yIndex-glyphPtr->top] = xIndex-1;
				continue; // keep going down
			}
			// try down-right; requires that right also be blank
			else if (xIndex < glyphPtr->right-1 && // not at right edge
				ISBLANK(yIndex, xIndex+1) && // right is blank
				ISBLANK(yIndex+1, xIndex+1) // down-right is blank
			) { // right-down works
				yIndex += 1;
				positions[yIndex-glyphPtr->top] = xIndex+1;
				continue; // keep going down
			}
			// try right
			else if (xIndex < glyphPtr->right-1 && // not at right edge
				ISBLANK(yIndex, xIndex+1) // right is blank
			) { // right works
				// do not change yIndex
				positions[yIndex-glyphPtr->top] = xIndex+1;
				continue;
			}
			// fprintf(stderr, "failed to find simple path starting at x=%d\n",
			//	xStartIndex);
			break;
		} // still working
		if (yIndex != glyphPtr->bottom-1) continue; // stopped before finishing
		// fprintf(stderr, "found path\n");
		return(positions);
	} // start path at top, xStartIndex
	// fprintf(stderr, "failed to find simple path starting anywhere\n");
	free(positions);
	return(0);
} // tryPathSeparationOld

/*
 * Δx = Δy/slant
 * xStartIndex = HEIGHT(glyph) / slant
 */

int *buildSlantedPath(glyph_t *glyph, int xStartIndex) {
	int height = HEIGHT(glyph);
	int *positions = (int *) malloc(sizeof(int)*height);
	int yOffset, xIndex;
	xIndex = xStartIndex;
	for (yOffset = 0; yOffset < height; yOffset += 1) {
		if (xStartIndex - xIndex < yOffset/slant) xIndex -= 1;
		xIndex = MAX(xIndex, glyph->left);
		positions[yOffset] = xIndex;
	}
	return positions;
} // buildSlantedPath

int *copyPath(int *path, int height) {
	if (path == 0) return 0;
	int *result = (int *)malloc(sizeof(int)*height);
	memcpy((char *)result, (char *) path, height*sizeof(int));
	return result;
} // copyPath

void shiftRight(int *path, int height) { // move one right
	for (; height > 0; height -= 1) {
		*path += 1; // shift
		path += 1; // next element in the path
	}
} // shiftPath

void finishPathSplit(glyph_t *glyphPtr, glyph_t *first, int *splitPath) {
// splitting glyphPtr into first and second.  First is already prepared,
// but we want to re-use glyphPtr, because it is already chained.
	glyph_t *second = (glyph_t *) calloc(1, sizeof(glyph_t));
	*second = *glyphPtr;
	second->leftPath = copyPath(splitPath, HEIGHT(second));
	narrowGlyph(second); // modifies paths/borders
	// BUG: if second has same left as first, we make no progress.
	second->tuple = newTuple();
	fillTuple(second, second->tuple);
	second->next = glyphPtr->next;
	free(glyphPtr->tuple);
	if (glyphPtr->leftPath) free(glyphPtr->leftPath);
	*glyphPtr = *first;
	glyphPtr->next = second;
	free(first);
} // finishPathSplit

// the following is under development.  It uses a flooding algorithm to find
// glyphs. 

#define MARK(row, col) do {image[AT(row,col)] = 2;} while(0)
#define UNMARK(row, col) do {image[AT(row,col)] = 1;} while(0)
#define ISMARKED(row, col) (image[AT(row,col)] == 2)
#define ISBLACK(row, col) (image[AT(row,col)] == 1)

void floodRecur(glyph_t *theGlyph, int row, int col, int topRow) {
	if (row < 0 || row >= height 
			|| col < leftBorder || col >= rightBorder) {
		return; // out of range
	}
	MARK(row, col);
	theGlyph->top = MIN(theGlyph->top, row);
	if (row-topRow < maxGlyphHeight) {
		// adjust boundary information
		theGlyph->bottom = MAX(theGlyph->bottom, row+1);
		theGlyph->leftPath[row-topRow] =
			MIN(theGlyph->leftPath[row-topRow], col);
		theGlyph->rightPath[row-topRow] =
			MAX(theGlyph->rightPath[row-topRow], col+1);
		theGlyph->left = MIN(theGlyph->left, col);
		theGlyph->right = MAX(theGlyph->right, col+1);
		// even if too high, keep filling, though.
	}
	if (row < height-1 && ISBLACK(row+1, col))
		floodRecur(theGlyph, row+1, col, topRow);
	if (0 < row && ISBLACK(row-1, col))
		floodRecur(theGlyph, row-1, col, topRow);
	if (col < width && ISBLACK(row, col+1))
		floodRecur(theGlyph, row, col+1, topRow);
	if (0 < col && ISBLACK(row, col-1))
		floodRecur(theGlyph, row, col-1, topRow);
	if (row < height-1 && col < width && ISBLACK(row+1, col+1))
		floodRecur(theGlyph, row+1, col+1, topRow);
	if (row < height-1 && 0 < col && ISBLACK(row+1, col-1))
		floodRecur(theGlyph, row+1, col-1, topRow);
	if (0 < row && col < width && ISBLACK(row-1, col+1))
		floodRecur(theGlyph, row-1, col+1, topRow);
	if (0 < row && 0 < col && ISBLACK(row-1, col-1))
		floodRecur(theGlyph, row-1, col-1, topRow);
} // floodRecur

int flood(glyph_t *answer, int row, int col) {
	answer->top = row;
	answer->bottom = row+1;
	answer->left = col;
	answer->right = col+1;
	int rowOffset;
	// initialize leftPath, rightPath with pseudo-data
	for (rowOffset = 0; rowOffset < maxGlyphHeight; rowOffset += 1) {
		answer->leftPath[rowOffset] = INFTY;
		answer->rightPath[rowOffset] = -1;
	}
	floodRecur(answer, row, col, row);
	if (answer->bottom - answer->top > maxGlyphHeight) {
		fprintf(stderr, "large glyph; should not happen.\n");
		abort(); // should not happen
	}
	int width = answer->right - answer->left;
	int height = answer->bottom - answer->top;
	if (width < minGlyphWidth || height < minGlyphHeight ||
			width * height < minGlyphArea) {
		return false;
	}
	return(true);
} // flood

int floodFindGlyphs(int column) {
	// computeBorders(column); // computes leftBorder, rightBorder (global)
	// lineHeaders = (lineHeaderList *) calloc(1, sizeof(lineHeaderList));
	int count = 0;
	glyph_t tmpGlyph;
	tmpGlyph.leftPath = malloc(sizeof(int)*maxGlyphHeight);
	tmpGlyph.rightPath = malloc(sizeof(int)*maxGlyphHeight);
	int row;
	for (row = 0; row < height; row += 1) {
		int col;
		for (col = leftBorder; col <= rightBorder; col += 1) {
			if (ISBLACK(row, col)) { // cell is black; need to flood
				if (flood(&tmpGlyph, row,col)) {;
					// fprintf(stdout, "found glyph\n");
					glyph_t *newGlyphPtr = insertGlyph(tmpGlyph.left,
						tmpGlyph.right, tmpGlyph.top, tmpGlyph.bottom, lineHeaders);
					newGlyphPtr->leftPath = copyPath(tmpGlyph.leftPath,
						tmpGlyph.bottom-tmpGlyph.top);
					newGlyphPtr->rightPath = copyPath(tmpGlyph.rightPath,
						tmpGlyph.bottom-tmpGlyph.top);
					count += 1;
					// if (count > 2000) return 1; // debug
				} // found a new glyph
			} // cell is black; need to flood
		} // each col
	} // each row
	free(tmpGlyph.leftPath);
	free(tmpGlyph.rightPath);
	return count;
} // floodFindGlyphs

void unMark() { // convert all marked cells back to simply black
	int row;
	for (row = 0; row < height; row += 1) {
		int col;
		for (col = leftBorder; col <= rightBorder; col += 1) {
			if (ISMARKED(row,col)) {
				UNMARK(row,col);
			}
		} // each col
	} // each row
} // unMark
