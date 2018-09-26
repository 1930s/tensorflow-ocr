/* display.c

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

void printRaster() {
	int row, col;
	for (row = 0; row < height; row += 1) {
		printf("row %4d\n", row);
		for (col = 0; col < width; col += 1) {
			fprintf(stdout, "%c",
				image[AT(row, col)] == 0 ? ' ' : '1');
		} // one column
		fprintf(stdout, "|\n");
	} // one row
} // printRaster

void printRegion(int top, int bottom, int left, int right) {
	int row, col;
	for (row = top; row < bottom; row += 1) {
		for (col = left; col < right; col += 1) {
			fprintf(stdout, "%c",
				image[AT(row, col)] == 0 ? ' ' : '1');
		} // each col
		fprintf(stdout, "|\n");
	} // each row
} // printRegion

void printGlyphs() {
	lineHeaderList *curLine;
	int lineIndex;
	lineIndex = 0;
	for (curLine = lineHeaders->next; curLine; curLine=curLine->next) {
		lineIndex += 1;
		fprintf(stdout, "line %d\n", lineIndex + 1);
		glyph_t *glyphPtr;
		for (glyphPtr = curLine->line->glyphs->next; glyphPtr; glyphPtr = glyphPtr->next) {
			printRegion(curLine->line->top, curLine->line->bottom,
				glyphPtr->left, glyphPtr->right);
			fprintf(stdout, "------\n");
		} // one glyph
	} // one line lineIndex
} // printGlyphs
