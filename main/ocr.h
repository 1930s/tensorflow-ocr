/* ocr.h

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

#include <tiffio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
// #include "/tmp/dmalloc-5.5.2/dmalloc.ho"

#define VERSION "2.0"
	// added shear to straighten rotated pages
	// fixed premature "free" that prevented proper horizontal split
	// when splitting wide characters, introduced minGlyphWidth space between.
	// todo: introduce min area (as well as min width, min height)

#define SAYERROR(variable, message) \
	if (!variable) { \
		fprintf(stderr, "error while %s\n", message); \
		exit(1); \
	}

#define AT(row, col) ((row)*(width) + col)
#ifndef MIN
#	define MIN(a, b) ((a) < (b) ? (a) : (b))
#	define MAX(a, b) ((a) < (b) ? (b) : (a))
#endif

// constants
#define INFTY 100000 // bigger than any X or Y value (conflicts with INFINITY)
#define GRID 5 // 5 x 5 grid
#define TUPLELENGTH (GRID*GRID + 2) // extra for aspect, relative height
#define BUCKETSIZE 20
#define MINMATCH 0.9 // distance squared limit to accept a match
#define GOODWIDTH 1250 // max width we like in the image
#define MINWIDTH 500 // min width for the image
#define MINGLYPHHEIGHT 10 // minimum height of a glyph
#define MINGLYPHWIDTH 10 // minimum width of a glyph
#define MAXGLYPHWIDTH 150 // bigger than this is a picture, not a glyph
#define MAXGLYPHHEIGHT 150 // bigger than this is a picture, not a glyph
#define GOODHEIGHT 300 // max height we like in the image
#define MAXTREEDEPTH 50 // max depth of the k-d tree
#define BIGDIST 1.0e10 // unreasonably large distance
#define PRECISION "%0.3f" // positions after the decimal
#define MAXSHEAR 4 // if worse, just use 0
#define false 0 
#define true 1 
#define SPACEFRACTION (0.60) // fraction of an average glyph we all a space
#define GOODMATCH (0.40) // distance for a match to be "good", not just ok.
#define SPLITTABLE (1.10) // how  much wider than normal is worth considering
	// splitting a glyph
#define LIGHTSUM 20 // this many pixels turned on counts as a "light" line
#define SHEARSCALE 100 // lower number gives better resolution, at higher cost
#define SLANT 3.65 // Δx = Δy/slant
#define CUTOFF 0.50 // histogram value; lower means more white

// types


typedef float *tuple_t; // points to an array of length TUPLELENGTH

typedef struct glyph_s {
	int left; // column of the left of the glyph
	int right; // column following the right of the glyph
	int top; // row of the top of the text line (same as in textLine)
	int bottom; // row following the bottom of the text line (same as in
		// textLine)
	int lineHeight; // height of the line in which this glyph appears
	int *leftPath, *rightPath; // if not null: a non-vertical delimiting path
	tuple_t tuple;
	float distance; // closest distance to known glyph, or -1 if uninitialized
	struct glyph_s *next;
} glyph_t;

typedef struct {
	int top; // row of the top of the text line
	int bottom; // row following the bottom of the text line
	int leftBorder, rightBorder;
	int splitTop, splitBottom; // boolean: artificially split at top or bottom
	glyph_t *glyphs; // there is a dummy at the start
	glyph_t *end; // the last glyph; points to dummy if none.
} textLine;

typedef struct lineHeaderStruct {
	textLine *line;
	struct lineHeaderStruct *next;
} lineHeaderList;

typedef struct {
	int count;
	tuple_t key[BUCKETSIZE];
	const char *values[BUCKETSIZE]; // for each tuple, if known.
} bucket_t;

typedef struct kd_node_s {
// internal nodes only
	int dimension;
	float discriminant;
	struct kd_node_s *left, *right;
// internal nodes and buckets
	tuple_t lowBounds, highBounds;
// buckets only (0 if an internal node)
	bucket_t *bucket;
} kd_node_t;

// for p_kd.c

typedef float *p_tuple_t; // points to an array of length DIMS

typedef struct {
	int count;
	p_tuple_t key[BUCKETSIZE];
	const glyph_t *glyphs[BUCKETSIZE]; // for each tuple, if known.
} p_bucket_t;

typedef struct p_kd_node_s {
// internal nodes only
	int dimension;
	float discriminant;
	struct p_kd_node_s *left, *right;
// internal nodes and buckets
	p_tuple_t lowBounds, highBounds;
// buckets only (0 if an internal node)
	p_bucket_t *bucket;
} p_kd_node_t;

// provided by readPicture.c
	extern tsize_t width; // in pixels
	uint32 *raster;
	extern tsize_t height; // in pixels
	extern char *image; // one char per pixel; values restricted to 0 and 1
	void readPicture();
	void shearPicture();
	extern const char *fileBase;
	extern const char *fontFile;
	void setFileBase(const char *base);
	int anotherPage();

// provided by segment.c
	extern int header; // number of blank pixels at top of page
	extern int spacer; // number of blank pixels between lines
	extern int lineWidth; // number of pixels in a text line
	extern int numLines; // number of text lines in the image
	int findLines();
	int floodFindGlyphs(int col);
	void wideGlyphs();
	void narrowGlyphs();
	void splitWideGlyphs();
	glyph_t *glyphAtXY(int row, int col, int verbose);
	extern int leftMargin, rightMargin, glyphWidth, glyphHeight;
	extern lineHeaderList *lineHeaders; // with a dummy at the front
	void freeLines();
	void unMark();

// provided by display.c
	void printRaster();
	void printGlyphs();
	void printRegion(int top, int bottom, int left, int right);

// provided by gtkDisplay.c
	void GUI();
	void displayText(void *theButton, int *visual);
	extern int redo; // 1 means redo same column; 0 means go on

// provided by categorize.c
	void fillTuple(glyph_t *glyph, tuple_t tuple);
	void refillTuple(glyph_t *glyph, tuple_t tuple);
	void setCategory(glyph_t *glyph, char *value);
	void buildTuples();
	void calculateDistance(glyph_t *glyphPtr);
	tuple_t newTuple();
	tuple_t copyTuple();

// provided by kd.c
	kd_node_t *buildEmptyTree();
	void insertTuple(kd_node_t *tree, tuple_t tuple, char *value);
	extern kd_node_t *categorization; // the root
	void getBucket(tuple_t tuple, bucket_t **bucket, int *index);
	float closestMatch(kd_node_t *tree, tuple_t probe, bucket_t **bucket,
		int *index, float betterThan);
	void printTree(kd_node_t *tree, int special, const char *message, int
		indent);
	void writeTuples();
	void readTuples();
	void freeTree(kd_node_t *tree);
	float ocrDistance2(tuple_t tuple);
	const char *ocrValue(tuple_t tuple);
	void printTuple(tuple_t tuple, int specialDimension);
	extern int RTL; // boolean, set to true if there are RTL characters.
	int hasRTL(const char *data);
	p_tuple_t p_newTuple(float x, float y);
	void normalizeTuple(tuple_t tuple);
#define OCRFAILS "▮"

// provided by p_kd.c
	p_kd_node_t *p_buildEmptyTree();
	void p_insertGlyph(const glyph_t *theGlyph);
	void p_getBucket(p_tuple_t tuple, p_bucket_t **bucket, int *index);
	float p_closestMatch(p_kd_node_t *tree, p_tuple_t probe, p_bucket_t
		**bucket, int *index, float betterThan);
	void p_printTree(p_kd_node_t *tree, int special, const char *message, int
		indent);
	void p_freeTree(p_kd_node_t *tree);
	float p_distance(p_tuple_t tuple);
	void printTuple(p_tuple_t tuple, int specialDimension);
	float p_closestGlyph(const glyph_t *theGlyph, const glyph_t **closestGlyph);
	void p_init();

// provided by training.c
	void writeTraining();
	void readTraining();

// provided by template.c
	void writeTemplate();
	void readTemplate();

// provided by main.c
	void simpleTest();
	extern int minGlyphWidth, minGlyphHeight;
	extern int maxGlyphWidth, maxGlyphHeight;
	extern int columns;
	extern int ignoreVertical;
	extern float spaceFraction, minMatch, minMatch2,
		goodMatch, goodMatch2, splittable, cutoff;
	extern float slant; // Δx = Δy/slant
	extern int useFlood;
	extern int mayCombine;
	extern int alwaysCombine; // for training purposes
	extern int minGlyphArea; // default INFTY
	extern int doTensorFlow; // enables TensorFlow
	extern int printTensorFlow;  //print TensorFlow in batch mode
	extern const char *tensorFile;
