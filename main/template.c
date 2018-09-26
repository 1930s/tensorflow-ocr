/*
template.c.

"Template" gives an answer to an image.  You can build it by running ocr on
the image and then by hand replacing mistakes with proper glyphs.

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
#include <assert.h>

#define MAXTEMPLATES 1000
int templateDataCount;
char *templateData[MAXTEMPLATES];

// called from GUI; reads the template data, inserting into the
// categorization, suppressing duplicates.
void readTemplate() {
	char buf[BUFSIZ];
	strcpy(buf, fileBase);
	strcat(buf, ".template");
	FILE *template = fopen(buf, "r");
	if (template == NULL) {
		perror(buf);
		return;
	}
	templateDataCount = 0;
	while (fgets(buf, BUFSIZ, template) != NULL) { // each line of the template
		assert(templateDataCount < MAXTEMPLATES-1);
		if (strlen(buf) < 5) continue; // not real data
		templateData[templateDataCount] = malloc(strlen(buf)+1);
		strcpy(templateData[templateDataCount], buf);
		templateDataCount += 1;
	} // each line of the template
	lineHeaderList *curLine;
	for (curLine = lineHeaders->next; curLine; curLine=curLine->next) {
		glyph_t *glyphPtr;
		// printf("line %d\n", lineNumber);
		for (glyphPtr = curLine->line->glyphs->next; glyphPtr; glyphPtr =
				glyphPtr->next) { // each glyph
			char buf[BUFSIZ];
			sprintf(buf, "%d %d %d %d ",
				glyphPtr->left, 
				glyphPtr->right, 
				glyphPtr->top, 
				glyphPtr->bottom); 
			templateData[templateDataCount] = buf; // pseudo-data
			int templateIndex;
			int length = strlen(buf);
			for (templateIndex = 0;
				strncmp(buf, templateData[templateIndex], length);
				templateIndex += 1); // find match
			if (templateIndex == templateDataCount) {
				// printf("no match for %s\n", buf);
			} else {
				char value[BUFSIZ];
				sscanf(templateData[templateIndex],
					"%*d %*d %*d %*d %s", value);
				char *toInsert = malloc(strlen(value)+1);
				strcpy(toInsert, value);
				setCategory(glyphPtr, toInsert);
			}
		} // each glyph
	} // each line
} // readTemplate

// called from GUI; writes the template data.
void writeTemplate() {
	char buf[BUFSIZ];
	strcpy(buf, fileBase);
	strcat(buf, ".template");
	FILE *template = fopen(buf, "w");
	if (template == NULL) {
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
			closestMatch(categorization, glyphPtr->tuple, &bucketPtr,
					&index, BIGDIST);
			fprintf(template, "%d %d %d %d %s\n", 
				glyphPtr->left,
				glyphPtr->right,
				glyphPtr->top,
				glyphPtr->bottom,
				bucketPtr->values[index]);
		} // each glyph
	} // each lineNumber
	fclose(template);
} // writeTemplate
