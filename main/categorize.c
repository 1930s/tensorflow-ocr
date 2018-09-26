/* categorize.c.

We categorize glyphs based on percentage of black in each of GRID x
GRID rectangular sections, plus one more datum: the width/height,
scaled to be in 0 .. 1.

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

#include <assert.h>
#include <math.h>
#include "ocr.h"

/*
static int floorCalc(float number) { // like built-in floor, but my design
	return((int)number);
} // floorCalc

static int ceiling(float number) {
	return(number > (int) number ? (int) (number+1) : (int) number);
} // ceiling
*/

tuple_t newTuple() {
	return (tuple_t) malloc(sizeof(float) * TUPLELENGTH);
} // newTuple

tuple_t copyTuple(tuple_t old) {
	tuple_t answer = newTuple();
	memcpy(answer, old, sizeof(float) * TUPLELENGTH);
	return answer;
} // copyTuple

void fillTuple(glyph_t *glyph, tuple_t tuple) {
	// divide the glyph into sections of GRID rows, GRID columns.  Each
	// section has a number of lines and positions.
	int row, column;
	// fprintf(stdout, "top is %d, bottom is %d\n", glyph->top, glyph->bottom);
	// fprintf(stdout, "left is %d, right is %d\n", glyph->left, glyph->right);
	float gheight = glyph->bottom - glyph->top;
	float gwidth = glyph->right - glyph->left;
	int firstLine[GRID], lastLine[GRID];
	float rowDelta = (gheight-1)/GRID;
	for (row = 0; row < GRID; row++) {
		firstLine[row] = glyph->top + lroundf(rowDelta * row);
		lastLine[row] = glyph->top + lroundf(rowDelta * (row+1)) + 1;
		// fprintf(stdout, "row %d is lines %d up to %d\n", row,
		// 	firstLine[GRID], lastLine[GRID]);
	} // each row
	float colDelta = (gwidth-1)/GRID;
	int firstPosition[GRID], lastPosition[GRID];
	for (column = 0; column < GRID; column++) {
		firstPosition[column] = glyph->left + lroundf(colDelta * column);
		lastPosition[column] = glyph->left +
			lroundf(colDelta * (column+1)) + 1;
		// fprintf(stdout, "column %d is positions %d up to %d\n", column,
		// 	firstPosition[GRID], lastPosition[GRID]);
	} // each column
	// printRegion(firstLine[0], lastLine[GRID-1],
	// 	firstPosition[0], lastPosition[GRID-1]);
	for (row = 0; row < GRID; row++) {
		for (column = 0; column < GRID; column++) {
			int totalCells = 0, filledCells = 0;
			int line, position;
			// fprintf(stdout, "\n");
			// printRegion(firstLine[row], lastLine[row],
			//	firstPosition[column], lastPosition[column]);
			for (line = firstLine[row]; line < lastLine[row]; line += 1) {
				// fprintf(stdout, "line %d\n", line);
				int fromTop = line - glyph->top;
				for (position = firstPosition[column]; position <
						lastPosition[column]; position += 1) {
					// fprintf(stdout, "position %d: %d\n", position,
					// 	image[AT(line, position)]);
					totalCells += 1;
					if (glyph->leftPath && glyph->leftPath[fromTop] > position) {
						// fprintf(stderr, "row %d, skipping to left of %d\n",
						// 	row, glyph->leftPath[row]);
						continue; // don't count points to left of leftPath
					}
					if (glyph->rightPath && glyph->rightPath[fromTop] <= position) {
						// fprintf(stderr, "row %d, skipping to right of %d\n",
						// 	row, glyph->rightPath[row]);
						continue; // don't count points to right of rightPath
					}
					if (image[AT(line, position)]) {
						filledCells += 1;
					}
				} // each position
			} // each line
			float content = ((float) filledCells)/totalCells;
			// we are only storing PRECISION decimal places, though.
			char buf[BUFSIZ];
			sprintf(buf, PRECISION, content);
			// fprintf(stdout, "reducing %0.10f to %s\n", content, buf);
			sscanf(buf, "%f", &tuple[row*GRID + column]);
			// fprintf(stdout, "%6.3f ", tuple[row*GRID + column]);
		} // each column
		// fprintf(stdout, "\n");
	} // each row
// aspect ratio
	tuple[GRID * GRID] = gheight < gwidth
		? gheight / (2.0*gwidth) : 1.0 - gwidth / (2.0*gheight);
	// fprintf(stderr, "height %g width %g\n", gheight, gwidth);
	// fprintf(stderr, "tuple aspect is %6.4f\n", tuple[GRID * GRID]);
// height above baseline
	tuple[GRID * GRID + 1] = // if ignoreVertical, we ignore in distSquared.
		(0.0 + gheight) /
			(glyph->lineHeight ? glyph->lineHeight : glyphHeight);
	// fprintf(stderr, "percentage height is %f\n", tuple[GRID*GRID]);
	normalizeTuple(tuple);
} // fillTuple

void refillTuple(glyph_t *glyph, tuple_t tuple) {
	// just the parts that don't have to do with pixels
	float gwidth = glyph->right - glyph->left;
	float gheight = glyph->bottom - glyph->top;
	tuple[GRID * GRID] = gheight < gwidth
		? gheight / (2.0*gwidth) : 1.0 - gwidth / (2.0*gheight);
	// fprintf(stderr, "tuple aspect is %6.4f\n", tuple[GRID * GRID]);
// height above baseline
	tuple[GRID * GRID + 1] = // if ignoreVertical, we ignore in distSquared.
		(0.0 + gheight) /
			(glyph->lineHeight ? glyph->lineHeight : glyphHeight);
	// fprintf(stderr, "percentage height is %f\n", tuple[GRID*GRID]);
	normalizeTuple(tuple);
} // refillTuple

void setCategory(glyph_t *glyph, char *value) {
	tuple_t tuple = newTuple();
	fillTuple(glyph, tuple);
	bucket_t *bucket;
	int index;
	getBucket(tuple, &bucket, &index);
	if (index < bucket->count) { // already present
		// fprintf(stderr, "setting category to %c\n", *value);
		bucket->values[index] = value;
		free(tuple);
	} else { // need to insert it
		// fprintf(stderr, "inserting %c\n", *value);
		insertTuple(categorization, tuple, value);
	}
} // setCategory

void buildTuples() {
	lineHeaderList *curLine;
	for (curLine = lineHeaders->next; curLine; curLine=curLine->next) {
		glyph_t *glyphPtr;
		for (glyphPtr = curLine->line->glyphs->next; glyphPtr; glyphPtr =
				glyphPtr->next) {
			glyphPtr->tuple = newTuple();
			fillTuple(glyphPtr, glyphPtr->tuple);
		} // each glyph
	} // each line
} // buildTuples

// fill the tuple if necessary and then calculate the distance, if necessary
void calculateDistance(glyph_t *glyphPtr) {
	if (!glyphPtr->tuple) {
		glyphPtr->tuple = newTuple();
		fillTuple(glyphPtr, glyphPtr->tuple);
	}
	if (glyphPtr->distance == -1) {
		glyphPtr->distance = sqrtf(ocrDistance2(glyphPtr->tuple));
	}
} // calculateDistance
