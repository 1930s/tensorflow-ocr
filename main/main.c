/*
main.c

Copyright © Raphael Finkel 2007-2010 raphael@cs.uky.edu  

meowmeow

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

#include <getopt.h>
#include "ocr.h"


// variables
int minGlyphWidth = MINGLYPHWIDTH;
int minGlyphHeight = MINGLYPHHEIGHT;
int maxGlyphWidth = MAXGLYPHWIDTH;
int maxGlyphHeight = MAXGLYPHHEIGHT;
float minMatch = MINMATCH;
float minMatch2 = MINMATCH*MINMATCH;
float spaceFraction = SPACEFRACTION;
float goodMatch = GOODMATCH;
float goodMatch2 = GOODMATCH*GOODMATCH;
float splittable = SPLITTABLE;
float slant = SLANT; // Δx = Δy/slant
float cutoff = CUTOFF; // histogram cutoff; smaller means more white space
int columns = 1;
int ignoreVertical = 0;
int noShear = 0;
int useFlood = 0;
int mayCombine = 1;
int alwaysCombine = false;
int minGlyphArea = 1;
int doTensorFlow = 0;

static void usage() {
fprintf(stderr,
	"Usage: ocr -f fontData [-T] [-t] [-h n] [-w n] [-s n] [-W n] [-H n] [image ...] \n"
	"\timage, image.tif, or image.tiff is the image file\n"
	"\timage.training is its training file.\n"
	"\tfontData associates glyph statistics with UTF8 strings.\n" 
	"\t-c n for n-column input.\n"
	"\t-t causes text output; it does not have an interactive component.\n"
	"\t-h n only considers glyphs at least n pixels tall (default %d)\n"
	"\t-w n only considers glyphs at least n pixels wide (default %d)\n"
	"\t-H n only considers glyphs at most n pixels high (default %d)\n"
	"\t-W n only considers glyphs at most n pixels wide (default %d)\n"
	"\t-m n max distance to consider a match (default %3.2f)\n"
	"\t-g n max distance to consider a good match (ordinary rectangle)\n"
		"\t\t(default %3.2f)\n"
	"\t-p n an unrecognized glyph this much wider than normal might split\n"
		"\t\t(default %3.2f)\n"
	"\t-s n a space must be at least this fraction of average glyph width\n"
		"\t\t(default %3.2f)\n"
	"\t-C n cutoff at this percent of full black"
		"\t\t(default %3.2f)\n"
	"\t-S do not try to shear to correct for rotation\n"
	"\t-L n use n as the italic correction; Δx = Δy/slant.\n"
		"\t\tBigger is more vertical. (default %3.2f)\n"
	"\t-x use flooding to determine glyph boundaries.\n"
	"\t-X do not try to combine or split.\n"
	"\t-A always combine horizontal overlaps, even if result is worse.\n"
	"\t-i ignore glyph's vertical placement in matching glyphs\n"
	"\t-d n minimum glyph area\n"
	"\t-T enables TensorFlow algorithms in batch mode\n",
		MINGLYPHHEIGHT, MINGLYPHWIDTH, MAXGLYPHHEIGHT, MAXGLYPHWIDTH, MINMATCH,
		GOODMATCH, SPLITTABLE, SPACEFRACTION, CUTOFF, SLANT
	);
exit(1);
} // usage

extern int fribidi_get_type_internal(int character);

void simpleTest() {
fprintf(stdout, "type of %d is 0%x\n", 0x05dc,
	fribidi_get_type_internal(0x05dc));
fprintf(stdout, "type of %d is 0%x\n", 0x0066,
	fribidi_get_type_internal(0x0066));
fprintf(stdout, "type of %d is 0%x\n", 0x0028,
	fribidi_get_type_internal(0x0028));
return;
// readTree(); // get training data
bucket_t *bucket;
int index;
glyph_t *glyph = lineHeaders->next->line->glyphs;
for (index = 0; index < 8; index += 1) {
	glyph = glyph->next;
}
tuple_t tuple = glyph->tuple;
float dist = closestMatch(categorization, tuple, &bucket,
	&index, BIGDIST);
fprintf(stdout, "distance is %7.5f from probe\n\t", dist);
printTuple(tuple, -1);
fprintf(stdout, "to solution at index %d\n\t", index);
if (index > -1) {
	printTuple(bucket->key[index], -1);
	fprintf(stdout, "with value [%s]\n", bucket->values[index]);
}
printTree(categorization, -1, "full tree", 0);
} // simpleTest

int main (int argc, char * const argv[]) {


fontFile = NULL;
int textOnly = false;
setvbuf(stderr, NULL, _IONBF, 0); // stderr comes out immediately
while (1) { // each option
	static struct option longOptions[] = {
		{"font", required_argument, 0, 0},
		{"doTensorFlow", no_argument, 0, 0},
		{"textout", no_argument, 0, 0},
		{"minHeight", required_argument, 0, 0},
		{"minWidth", required_argument, 0, 0},
		{"spaceFraction", required_argument, 0, 0},
		{"minMatch", required_argument, 0, 0},
		{"maxHeight", required_argument, 0, 0},
		{"maxWidth", required_argument, 0, 0},
		{"ignoreVertical", no_argument, 0, 0},
		{0, 0, 0, 0}
	};
	int optionIndex;
	int c = getopt_long(argc, argv, "f:Tth:w:s:m:H:W:g:p:c:iSL:xC:XAd:",
		longOptions, &optionIndex);
	if (c == -1) break; // no more options
	switch (c) {
		case 0: // long option
			fprintf(stdout, "long option %s\n",
				longOptions[optionIndex].name);
			if (optarg)
				fprintf(stdout, "\twith argument %s\n", optarg);
			break;
		case 'f': 
			fontFile = optarg; break;
		case 't':
			textOnly = true; break;
		case 'w':
			minGlyphWidth = atoi(optarg); break;
		case 'h':
			minGlyphHeight = atoi(optarg); break;
		case 'm':
			minMatch = atof(optarg); break;
		case 'W':
			maxGlyphWidth = atoi(optarg); break;
		case 'H':
			maxGlyphHeight = atoi(optarg); break;
		case 's':
			spaceFraction = atof(optarg); break;
		case 'g':
			goodMatch = atof(optarg);
			goodMatch2 = goodMatch * goodMatch;
			break;
		case 'p':
			splittable = atof(optarg);
			break;
		case 'C':
			cutoff = atof(optarg);
			fprintf(stderr, "Using darkness cutoff %3.2f\n", cutoff);
			break;
		case 'c':
			columns = atoi(optarg);
			fprintf(stderr, "Using %d columns\n", columns);
			break;
		case 'i':
			ignoreVertical = true;
			break;
		case 'S':
			noShear = true;
			break;
		case 'L':
			slant = atof(optarg);
			fprintf(stderr, "Using %f for slant\n", slant);
			break;
		case 'x':
			fprintf(stderr, "flooding to identify glyphs\n");
			useFlood = true;
			break;
		case 'X':
			fprintf(stderr, "not combining or splitting\n");
			mayCombine = false;
			break;
		case 'A':
			fprintf(stderr, "always combining horizontal\n");
			alwaysCombine = true;
			break;
		case 'd':
			minGlyphArea = atoi(optarg);
			fprintf(stderr, "Minimum acceptable area %d\n", minGlyphArea);
			break;
		case 'T':
			//fprintf(stdout, "Meowy McMeowskies\n");
			doTensorFlow = 1; break;
		case '?':
			fprintf(stdout, "unrecognized option\n");
			usage();
			break;

	
	} // switch
} // each option
if (fontFile == NULL) {
	usage();
}
readTuples();
while (optind < argc) { // each TIFF file
	fileBase = argv[optind++];
	char *dotPos = rindex(fileBase, '.');
	if (dotPos) *dotPos = 0; // remove extension 
	do {
		// printf("reading picture\n");
		if(!doTensorFlow)
			fprintf(stdout, "%s\n", fileBase);
		fprintf(stderr, "%s\n", fileBase);
		readPicture();
		if (!noShear) shearPicture();
		// p_init();
		// printRaster();
		int col;
		int startCol, lastCol, increment;
		if (RTL) {
			// fprintf(stderr, "RTL mode\n");
			startCol = columns-1;
			lastCol = -1;
			increment = -1;
		} else {
			startCol = 0;
			lastCol = columns;
			increment = 1;
		}
		for (col = startCol; col != lastCol; col += increment) {
			again:
			// fprintf(stderr, "column %d/%d\n", col+1, columns);
			if (!findLines(col)) continue;
			// printf("building tuples\n");
			// mtrace();
			buildTuples();
			// printf("wide Glyphs\n");
			if (mayCombine) splitWideGlyphs();
			if (mayCombine) {
				narrowGlyphs(); // combine adjacent narrow glyphs if reasonable
			}
			// printGlyphs();
			// simpleTest();
			int visual = 0;
			// printf("displaying\n");
			if (textOnly) {
				displayText(NULL, &visual);
			} else {
				redo = 0; // unless we learn otherwise.
				GUI(col);
				if (redo) {
					// fprintf(stdout, "redoing\n");
					freeLines();
					if (useFlood) unMark();
					goto again;
				} else {
					// fprintf(stdout, "not redoing\n");
				}
			} // GUI
			freeLines();
			// muntrace();
		} // each column
	} while (anotherPage());
} // each file
return(0);
} // main
