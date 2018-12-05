/*
readPicture.c: read tiff file into internal format

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

#include <sys/stat.h>
#include <math.h>
#include "ocr.h"

// for the world
char *image;
tsize_t width, height;
const char *fileBase = 0; // file name of picture (just the base part)
const char *fontFile = 0; // file name of font data
const char *tensorFile=0;

// just for here
uint32 *raster = NULL;

TIFF* myTiff = NULL;

void readPicture() {
	if (myTiff == NULL) { // first time
		if (!fileBase) {
			fprintf(stderr, "file base has not been set.  Bug.\n");
			exit(1);
		}
		char *fileName = malloc(strlen(fileBase) + 6);
		strcpy(fileName, fileBase);
		struct stat dummyBuf;
		if (!stat(fileName, &dummyBuf)) {
			myTiff = TIFFOpen(fileName, "r");
		}
		if (!myTiff) {
			strcat(fileName, ".tif");
			if (!stat(fileName, &dummyBuf)) {
				myTiff = TIFFOpen(fileName, "r");
			}
		}
		if (!myTiff) {
			strcat(fileName, "f");
			myTiff = TIFFOpen(fileName, "r");
		}
		// fprintf(stdout, "about to read %s\n", fileName);
		// fprintf(stdout, "finished reading %s\n", fileName);
		if (myTiff == NULL) {
			perror(fileName);
			exit(1);
		}
		free(fileName);
	} // first time
// get statistics
	int ok;
	ok = TIFFGetField(myTiff, TIFFTAG_IMAGEWIDTH, &width);
	SAYERROR(ok, "getting width");
	ok = TIFFGetField(myTiff, TIFFTAG_IMAGELENGTH, &height);
	SAYERROR(ok, "getting height");
	// fprintf(stdout, "Image is %dx%d pixels\n", width, height);
// build and populate raster
	// fprintf(stdout, "allocating %d x %d * %d = %d\n",
	// 	width, height, sizeof(uint32),
	// 	width * height * sizeof(uint32));
	raster = (uint32*) _TIFFmalloc(width * height * sizeof(uint32));
	SAYERROR(raster, "allocating raster");
	ok = TIFFRGBAImageOK(myTiff, NULL);
	SAYERROR(ok, "TIFFRGBAImageOK");
	typedef struct _TIFFRGBAImage TIFFRGBAImage;
	TIFFRGBAImage img;
	ok = TIFFRGBAImageBegin(&img, myTiff, false, NULL);
	SAYERROR(ok, "TIFFRGBAImageBegin");
	ok = TIFFRGBAImageGet(&img, raster, width, height);
	SAYERROR(ok, "TIFFRGBAImageGet");
	TIFFRGBAImageEnd(&img);
// vertically flip the raster
	int row, col;
	for (row = 0; row <= height/2; row += 1) {
		for (col = 0; col < width; col += 1) {
			uint32 tmp;
			tmp = raster[AT(row, col)];
			raster[AT(row, col)] = raster[AT(height-row-1, col)];
			raster[AT(height-row-1, col)] = tmp;
		}
	}
	image = (char *) _TIFFmalloc(width * height);
	uint32 *rasterPtr = raster;
	int cutoffTotal = lroundf(3*256*cutoff);
		// 3 colors, 256 maxval, cutoff default 0.5
	for (row = 0; row < height; row += 1) {
		for (col = 0; col < width; col += 1) {
			int total = TIFFGetR(*rasterPtr) + TIFFGetB(*rasterPtr) +
				TIFFGetG(*rasterPtr);
			// image[AT(height-row-1, col)] = (total > 128*3) ? 0 : 1;
			image[AT(row, col)] = (total > cutoffTotal) ? 0 : 1;
			/*
			if (total > 128*3) {
				fprintf(stderr, "[%d,%d]", col, height-row-1);
			}
			*/
			rasterPtr += 1;
		} // one column
	} // one row
	// fprintf(stderr, "\n");
} // readFile

/* shear: positive shear means slanted NW-SE; negative is SW-NE.
*/

int countLight(int shear) { // how many light lines at this shear?
	int answer = 0;
	int row;
	for (row = 0; row < height; row += 1) {
		int rowSum = 0;
		int col;
		for (col = 0; col < width; col += 1) {
			int effectiveRow = row + (col*shear)/SHEARSCALE;
			if (effectiveRow < 0 || effectiveRow >= height) {
				// off the edge; don't count as zero row.
				// rowSum += 1; // actually better to count as zero row
			} else {
				rowSum += image[AT(effectiveRow, col)];
			}
			if (rowSum > LIGHTSUM) break;
		} // each col
		answer += (rowSum > LIGHTSUM) ? 0 : 1;
	} // each row
	// fprintf(stderr, "shear %d gives us %d light lines out of %d\n",
	// 	shear, answer, height);
	return(answer);
} // countLight

void shearPicture() {
	int row, col;
	// find a reasonable definition of "light"
	int bestShear; // the shear that gives us the most "light" horizontal lines.
	int bestLights = 0; // the number of "light" horizontal lines.
	int shear, newLights;
	bestLights = countLight(0);
	bestShear = 0;
	shear = 1;
	newLights = countLight(shear);
	// fprintf(stderr, "newLights %d, bestLights %d\n", newLights, bestLights);
	if (newLights > bestLights) { 
		while (newLights > bestLights && shear <= MAXSHEAR) {// walk upwards
			bestShear = shear;
			bestLights = newLights;
			shear += 1;
			// fprintf(stderr, "shear going up to %d\n", shear);
			newLights = countLight(shear);
		}
		if (shear > MAXSHEAR) {
			bestShear = 0;
		}
	} else {
		shear = -1;
		newLights = countLight(shear);
		// fprintf(stderr, "newLights %d, bestLights %d\n", newLights, bestLights);
		while (newLights > bestLights && shear >= -MAXSHEAR) {// walk downwards
			bestShear = shear;
			bestLights = newLights;
			shear -= 1;
			newLights = countLight(shear);
		}
		if (shear < -MAXSHEAR) {
			bestShear = 0;
		}
	}
	fprintf(stderr, "best shear is %d; we get %d light lines\n", bestShear,
		bestLights);
	// adjust both the raster and the image.
	for (col = 0; col < width; col += 1) {
		int correction = (col*bestShear)/SHEARSCALE;
		if (correction == 0) continue;
		if (correction < 0) { // move row down by correction
			for (row = height-1+correction; row >= 0; row -= 1) {
				image[AT(row-correction,col)] = image[AT(row,col)];
				raster[AT(row-correction,col)] = raster[AT(row,col)];
			} // each row
			// clear the remainder
			for (row = -1; row > -1+correction; row -= 1) {
				image[AT(row-correction,col)] = 0;
				raster[AT(row-correction,col)] = 0;
			}
		} else { // move row up by correction
			for (row = correction; row < height; row += 1) {
				image[AT(row-correction,col)] = image[AT(row,col)];
				raster[AT(row-correction,col)] = raster[AT(row,col)];
			} // each row
			// clear the remainder
			for (row = height; row < height+correction; row += 1) {
				image[AT(row-correction,col)] = 0;
				raster[AT(row-correction,col)] = 0;
			}
		}
	} // each col
	for (row = 0; row < height; row += 1) {
		int correction = (row*bestShear)/SHEARSCALE;
		if (correction == 0) continue;
		int effectiveWidth = width - abs(correction);
		if (correction < 0) {
			memmove(&image[AT(row,0)], &image[AT(row,-correction)], effectiveWidth * sizeof(image[0]));
			memmove(&raster[AT(row,0)], &raster[AT(row,-correction)], effectiveWidth * sizeof(raster[0]));
		} else {
			memmove(&image[AT(row,correction)], &image[AT(row,0)], effectiveWidth * sizeof(image[0]));
			memmove(&raster[AT(row,correction)], &raster[AT(row,0)], effectiveWidth * sizeof(raster[0]));
		}
	} // each row
} // shearPicture

int anotherPage() {
	_TIFFfree(image);
	_TIFFfree(raster);
	if (TIFFReadDirectory(myTiff)) {
		// fprintf(stderr, "There are more pages in this file.\n");
		return(true);
	}
	TIFFClose(myTiff);
	myTiff = NULL;
	return(false);
} // anotherPage
