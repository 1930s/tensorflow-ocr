/*
training.c.

"Training data" associates glyphs (indicated by position of the
upper-left corner in the image file) with values (UTF8 strings).  It
does not involve the tuple representation.  Training data therefore
lets us redesign the tuple representation without losing training.
We don't store training data internally as such.  When we output
training data, we detect it by noticing glyphs whose tuples are found
in the categorization.  We can also read training data into a fresh
categorization.

Copyright © Raphael Finkel 2007-2010.

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

// called from GUI; saves the training data.  It may output duplicates,
// because it only checks that the glyph is exactly represented.
void writeTraining() {
	char buf[BUFSIZ];
	strcpy(buf, fileBase);
	strcat(buf, ".training");
	fprintf(stdout, "writing %s\n", buf);
	FILE *training = fopen(buf, "w");
	if (training == NULL) {
		perror(buf);
		return;
	}
	lineHeaderList *curLine;
	for (curLine = lineHeaders->next; curLine; curLine=curLine->next) {
		glyph_t *glyphPtr;
		bucket_t *bucketPtr;
		int index;
		for (glyphPtr = curLine->line->glyphs->next; glyphPtr;
				glyphPtr = glyphPtr->next) {
			float dist = closestMatch(categorization, glyphPtr->tuple,
				&bucketPtr, &index, BIGDIST);
			if (dist <= 0.001) { // this glyph is part of training
				fprintf(training, "%d %d %d %d %s\n", 
					glyphPtr->left,
					glyphPtr->right,
					glyphPtr->top,
					glyphPtr->bottom,
					bucketPtr->values[index]);
			} // part of training 
			else {
				// fprintf(stderr, "not training (distance %f): %s\n",
				// 	dist, bucketPtr->values[index]);
			}
		} // each glyph
	} // each lineNumber
	fclose(training);
} // writeTraining

// called from GUI; reads the training data, inserting into the
// categorization, suppressing duplicates.
void readTraining() {
	char buf[BUFSIZ];
	strcpy(buf, fileBase);
	strcat(buf, ".training");
	FILE *training = fopen(buf, "r");
	if (training == NULL) {
		perror(buf);
		return;
	}
	if (categorization) freeTree(categorization);
	categorization = buildEmptyTree();
	int left, right, top, bottom;
	while (fgets(buf, BUFSIZ, training) != NULL) {
		sscanf(buf, " %d %d %d %d %s\n", 
			&left, &right, &top, &bottom, buf);
		// fprintf(stdout, "training: %d %d: %s\n", left, top, buf);
		glyph_t *glyphPtr = glyphAtXY(left, top, 0);
		if (glyphPtr == NULL) {
			fprintf(stderr,
				"training data at (%d, %d) does not correspond to a glyph\n",
				left, top);
			continue;
		}
		char *value = malloc(strlen(buf) + 1);
		strcpy(value, buf);
		if (hasRTL(value)) {
			RTL = true;
		}
		bucket_t *bucketPtr;
		int index;
		if (closestMatch(categorization, glyphPtr->tuple, &bucketPtr,
				&index, BIGDIST) != 0) { // not a duplicate
			tuple_t tupleCopy = copyTuple(glyphPtr->tuple);
			insertTuple(categorization, tupleCopy, value);
		} // not a duplicate
	} // each line of training
	// printTree(categorization, -1, "after reading training", 0);
	fclose(training);
} // readTraining
