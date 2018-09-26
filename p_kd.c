/*
p_kd.c

Copyright © Raphael Finkel 2010 raphael@cs.uky.edu  

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

/* This set of routines handles planar ("p") closeness, so we can group glyphs
 * into lines based on what they are close to.
 */

#include <math.h>
#include "ocr.h"

#define VERYBIG 1000000 // actual values are X and Y coordinates in an image
#define DIMS 2 // just a plane

static p_kd_node_t *layout = NULL; // the root

p_tuple_t p_newTuple(float x, float y) {
	p_tuple_t answer;
	answer = (p_tuple_t) malloc(sizeof(float) * DIMS);
	*answer = x;
	*(answer+1) = y;
	return(answer);
} // p_newTuple

static float distSquared(p_tuple_t tuple1, p_tuple_t tuple2) {
	int dimension;
	float answer = 0.0;
	for (dimension = 0; dimension < DIMS; dimension += 1) {
		// fprintf(stderr, "\t\tdim %d: %f vs %f\n",
		// 	dimension, tuple1[dimension], tuple2[dimension]);
		float diff = tuple1[dimension] - tuple2[dimension];
		answer += diff * diff;
	} // each dimension
	return answer;
} // distSquared

static p_tuple_t p_copyTuple(p_tuple_t old) {
	p_tuple_t answer = (p_tuple_t) malloc(sizeof(float) * DIMS);
	memcpy(answer, old, sizeof(float) * DIMS);
	return answer;
} // p_copyTuple

static int ballWithinBounds(p_tuple_t probe, p_kd_node_t *treeNode,
		float trueDist) {
	int dimension;
	// fprintf(stdout, "\tball within bounds test, distance is %7.3f\n", trueDist);
	for (dimension = 0; dimension < DIMS; dimension += 1) {
		if (probe[dimension] - trueDist < treeNode->lowBounds[dimension] ||	
				probe[dimension] + trueDist > treeNode->highBounds[dimension]) {
			// fprintf(stdout,
			// 	"\tfailing at dimension %d; probe has %7.3f, bounds are "
			// 	"[%7.3f,%7.3f]\n", dimension, probe[dimension],
			// 	treeNode->lowBounds[dimension],
			// 	treeNode->highBounds[dimension]);
			return(false); // not within bounds
			}
	} // each dimension
	// fprintf(stdout, "\twithin bounds\n");
	return(true);
} // ballWithinBounds

// return whether a ball of radius trueDist around probe intersects the region
// controlled by treeNode.
static int ballOverlapBounds(p_tuple_t probe, p_kd_node_t *treeNode,
		float trueDist) {
	int dimension;
	p_tuple_t closestCorner = (p_tuple_t) malloc(sizeof(float) * DIMS);
	for (dimension = 0; dimension < DIMS; dimension += 1) {
		if (probe[dimension] < treeNode->lowBounds[dimension]) {
			closestCorner[dimension] = treeNode->lowBounds[dimension];
		} else if (probe[dimension] > treeNode->highBounds[dimension]) {
			closestCorner[dimension] = treeNode->highBounds[dimension];
		} else {
			closestCorner[dimension] = probe[dimension];
		}
	} // each dimension
	int answer = distSquared(probe, closestCorner) < trueDist * trueDist;
	// fprintf(stdout, "\tballOverlapBounds: closest distance is %7.3f; "
	// 	"true distance is %7.3f\n", sqrtf(distSquared(probe, closestCorner)),
	// 	trueDist);
	free(closestCorner);	
	return(answer);
} // ballOverlapBounds

// Return the closest match to the probe, within the given tree, as a pair
// (bucket, index); return -1 if there is nothing at all in the tree.  Also
// return the true distance to that closest match.  If, though, the closest
// match is not better than betterThan, return BIGDIST.
float p_closestMatch(p_kd_node_t *tree, p_tuple_t probe, p_bucket_t **bucket,
		int *index, float betterThan) {
	int closestIndex = -1;
	// fprintf(stdout, "\nstarting p_closestMatch test\n");
// initial dive to a bucket
	p_kd_node_t *stack[MAXTREEDEPTH];
	int stackSize = 0;
	while (!(tree->bucket)) { // internal node
		if (stackSize >= MAXTREEDEPTH) {
			fprintf(stderr, "MAXTREEDEPTH %d needs to be increased\n",
				MAXTREEDEPTH);
			exit(1);
		}
		stack[stackSize++] = tree;
		// fprintf(stdout, "discriminant %5.5f in dimension %d; "
		// 	" probe %5.5f; diving %s\n",
		// 	tree->discriminant, tree->dimension, probe[tree->dimension], 
		// 	probe[tree->dimension] <= tree->discriminant ? "left" : "right");
		tree = (probe[tree->dimension] <= tree->discriminant)
			? tree->left : tree->right;
	} // while internal node
// compare probe with all elements of the bucket
	int count;
	float distance2 = BIGDIST;
	// fprintf(stderr, "p_closestMatch: bucket has %d elements\n",
	//		tree->bucket->count);
	for (count = 0; count < tree->bucket->count; count += 1) { // each tuple
		float dist2 = distSquared(probe, tree->bucket->key[count]);
		// fprintf(stderr, "\tdist2 = %f\n", dist2);
		if (dist2 < distance2) {
			closestIndex = count;
			distance2 = dist2;
		}
	} // each tuple
	*bucket = tree->bucket;
	*index = closestIndex;
	float radius;
	if (distance2 < betterThan * betterThan) {
		radius = sqrtf(distance2);
		// fprintf(stdout, "bucket did better than %6.3f: %6.3f\n",
		// 	betterThan, radius);
	} else {
		radius = betterThan;
		// fprintf(stdout, "bucket has no better than %6.3f\n", betterThan);
	}
// are we within bounds?
	// fprintf(stderr, "p_closestMatch: radius %f\n", radius);
	while (stackSize) { // move back up the tree if necessary
		// fprintf(stdout,
		// 	"\tstack depth %d; checking ball within bounds; radius is %6.3f\n",
		// 	stackSize, radius);
		if (ballWithinBounds(probe, tree, radius)) {
			// fprintf(stdout, "\tsucceeded\n");
			break;
		}
		p_kd_node_t *parent = stack[--stackSize];
		// fprintf(stdout, "\tfailed; my sibling is on the %s\n",
		// 	(tree == parent->left) ? "right" : "left");
		p_kd_node_t *sibling =
			(tree == parent->left) ? parent->right : parent->left;
		if (ballOverlapBounds(probe, sibling, radius)) { // search sibling
			// fprintf(stdout, "\tsibling is worth checking\n");
			p_bucket_t *siblingBucket;
			int siblingIndex;
			float siblingDist = p_closestMatch(sibling, probe, &siblingBucket,
				&siblingIndex, radius);
			if (siblingDist < radius) { // found a better match in sibling
				// fprintf(stdout, "\tsibling is better; dist is %6.3f\n",
				// 	siblingDist);
				*bucket = siblingBucket;
				*index = siblingIndex;
				radius = siblingDist;
			} // sibling is better
			// else { fprintf(stdout, "\tSibling not better\n");}
		} // search sibling
		// else { fprintf(stdout, "\tNot worth checking sibling\n");}
		tree = parent;
	} // parent stack not empty
	// fprintf(stdout, "\tReturning; %s\n",
	// 	radius < betterThan ? "improved" : "no better");
	return (radius < betterThan ? radius : BIGDIST);
} // p_closestMatch

float p_distance(p_tuple_t tuple) {
	p_bucket_t *bucket;
	int index;
	p_closestMatch(layout, tuple, &bucket, &index, BIGDIST);
	if (index == -1) return(BIGDIST);
	return (distSquared(tuple, bucket->key[index]));
} // p_distance

static int maxSpreadDimension(p_tuple_t *key, int count) {
	float maxSpread = 0;
	int answerDimension = 0;
	p_tuple_t mins = (p_tuple_t) malloc(sizeof(float) * DIMS);
	p_tuple_t maxes = (p_tuple_t) malloc(sizeof(float) * DIMS);
	if (count == 0) {
		fprintf(stderr, "Cannot compute dimensions of an empty list\n");
		exit(1);
	}
	memcpy(mins, key[0], sizeof(float) * DIMS);
	memcpy(maxes, key[0], sizeof(float) * DIMS);
	int tupleIndex;
	int dimension;
	for (tupleIndex = 1; tupleIndex < count; tupleIndex += 1) {
		p_tuple_t tuple = key[tupleIndex];
		for (dimension = 0; dimension < DIMS; dimension += 1) {
			mins[dimension] = MIN(mins[dimension], tuple[dimension]);
			maxes[dimension] = MAX(maxes[dimension], tuple[dimension]);
		}
	} // each tuple
	for (dimension = 0; dimension < DIMS; dimension += 1) {
		float spread = maxes[dimension] - mins[dimension];
		if (spread > maxSpread) {
			// fprintf(stdout, "spread in dimension %d is %7.3f\n", dimension,
			// 	spread);
			maxSpread = spread;
			answerDimension = dimension;
		}
	}
	return(answerDimension);
} // maxSpreadDimension

#define swap(index1, index2) \
	do { \
		p_tuple_t tmpContent = key[(index1)]; \
		key[(index1)] = key[(index2)]; \
		key[(index2)] = tmpContent; \
		const glyph_t *tmpValue = glyphs[(index1)]; \
		glyphs[(index1)] = glyphs[(index2)]; \
		glyphs[(index2)] = tmpValue; \
	} while (0)

// partition the bucket between lowIndex and highIndex (inclusive) with respect
// to the given dimension, returning the pivot index.  Use Lomuto's algorithm
// (Jon Bentley (2000, p 117))
static int partition(p_tuple_t *key, const glyph_t **glyphs, int dimension,
		int lowIndex, int highIndex) {
	int combIndex;
	float pivotValue = key[lowIndex][dimension];
	int lastSmallIndex = lowIndex;
	for (combIndex = lowIndex+1; combIndex <= highIndex; combIndex += 1) {
		// tuples[lowIndex] is the pivot
		// tuples[lowIndex+1 .. lastSmallIndex] are < pivotValue
		// tuples[lastSmallIndex+1 .. combIndex-1] are >= pivotValue
		if (key[combIndex][dimension] <= pivotValue) {
			lastSmallIndex += 1;
			swap(lastSmallIndex, combIndex);
		} // swap
	} // each combIndex
	// final swap
	swap(lastSmallIndex, lowIndex);
	return(lastSmallIndex);
} // partition

// we would like to use the kth-smallest algorithm, but it is not guaranteed to
// place all equivalent values next to each other, and we need that guarantee,
// at least for the median.  So instead, we resort to quicksort of the entire
// array on the given dimension.

// sort by key, from lowIndex to highIndex (inclusive), in the given
// dimension, carrying values along simultaneously.
static void quickSort(p_tuple_t *key, const glyph_t **glyphs,
		int dimension, int lowIndex, int highIndex) {
	if (highIndex - lowIndex <= 0) return; // base case
	int partitionAt = partition(key, glyphs, dimension, lowIndex,
		highIndex);
	quickSort(key, glyphs, dimension, lowIndex, partitionAt - 1);
	quickSort(key, glyphs, dimension, partitionAt + 1, highIndex);
} // quickSort

// returns -1 if all values are the same, so there is no median.
static int median(p_tuple_t *key, const glyph_t **glyphs,
		int dimension, int lowIndex, int highIndex) {
	quickSort(key, glyphs, dimension, lowIndex, highIndex);
	int answer = (lowIndex + highIndex) / 2;
	if (0) { // debugging only
		int index;
		fprintf(stdout, "in dimension %d over [%d,%d]: ", dimension, lowIndex,
			highIndex);
		for (index = lowIndex; index <= highIndex; index += 1) {
			fprintf(stdout, "%d:%7.4f[%d] ", index, key[index][dimension],
				(int) glyphs[index]);
		}
		fprintf(stdout, "\n");
	}
	// it could be that the key at answer are constant nearby; move to end
	// of the constant range.
	float target = key[answer][dimension];
	// fprintf(stdout, "suggesting %d in [%d, %d]/%d: %7.3f\n", answer,
	//	lowIndex, highIndex, dimension, target);
	int mover;
	for (mover = answer; mover < highIndex; mover += 1) { // move up
		if (key[mover+1][dimension] > target) break;
		// fprintf(stdout, "moving up to %d\n", mover+1);
	}
	if (mover < highIndex) return(mover); // last tying element
	// fprintf(stdout, "too high; trying %d\n", answer - 1);
	// from answer to highIndex is constant; we have to move down instead
	for (mover = answer-1; mover >= lowIndex; mover -= 1) { // move down
		if (key[mover][dimension] < target) break;
		// fprintf(stdout, "moving down to %d\n", mover-1);
	}
	if (mover >= lowIndex) return(mover); // last pre-tying element
	fprintf(stderr,
		"key in dimension %d from %d to %d are constant %6.3f\n",
		dimension, lowIndex, highIndex, target);
	return(-1); // failure
} // median

// build a new bucket, copying some information from an old bucket (from
// startIndex to endIndex, inclusive), initializing the bounds as copies from
// the parent.
static p_kd_node_t *buildBucket(
	p_bucket_t *oldBucket, int startIndex, int endIndex, p_kd_node_t *parent) {
	// fprintf(stdout, "building a bucket from %d through %d\n",
	// 	startIndex, endIndex);
	p_kd_node_t *answer = (p_kd_node_t *)malloc(sizeof(p_kd_node_t));
	answer->lowBounds = p_copyTuple(parent->lowBounds);
	answer->highBounds = p_copyTuple(parent->highBounds);
	answer->bucket = (p_bucket_t *) malloc(sizeof(p_bucket_t));
	answer->bucket->count = endIndex - startIndex + 1;
	if (!oldBucket) return(answer); // empty input, empty copy
	// fprintf(stdout, "copying key\n");
	memcpy(answer->bucket->key, oldBucket->key+startIndex,
		answer->bucket->count * sizeof(float *));
	memcpy(answer->bucket->glyphs, oldBucket->glyphs+startIndex,
		answer->bucket->count * sizeof(char *));
	return(answer);
} // buildBucket

// debugging support
void p_printTuple(p_tuple_t tuple, int specialDimension) {
	int dimension;
	for (dimension = 0; dimension< DIMS; dimension+=1 ) {
		if (dimension == specialDimension) {
			fprintf(stdout, "(%6.3f) ", tuple[dimension]);
		} else {
			fprintf(stdout, "%6.3f ", tuple[dimension]);
		}
		// if (dimension >= 3) break; // debug
	} // each dimension
	fprintf(stdout, "\n");
} // p_printTuple

static void p_printBucket(
	p_bucket_t *bucket, int dimension, const char *message) {
	if (strlen(message)) fprintf(stdout, "%s ", message);
	fprintf(stdout, "bucket ");
	int elementIndex;
	for (elementIndex = 0; elementIndex < bucket->count; elementIndex += 1) {
		fprintf(stdout, "[%d] ", (int) bucket->glyphs[elementIndex]);
		printTuple(bucket->key[elementIndex], dimension);
	} // each element
	fprintf(stdout, "\n");
} // p_printBucket

void p_printTree(p_kd_node_t *tree, int dimension, const char *message, 
		int indent) {
	fprintf(stdout, "%s ", message);
	int index;
	for (index = indent; index; index -= 1) fprintf(stdout, "  ");
	if (tree->bucket) {
		p_printBucket(tree->bucket, dimension, "");
	} else {
		// fprintf(stdout, "my low bounds are ");
		// printTuple(tree->lowBounds, -1);
		// fprintf(stdout, "my high bounds are ");
		// printTuple(tree->highBounds, -1);
		fprintf(stdout, "dimension %d, discriminant %6.3f\n", tree->dimension,
			tree->discriminant);
		p_printTree(tree->left, dimension, "", indent+1);
		p_printTree(tree->right, dimension, "", indent+1);
	}
} // p_printTree

void p_insertTuple(p_kd_node_t *tree, p_tuple_t tuple, const glyph_t *theGlyph) {
	p_kd_node_t *parent = 0;
	while (!tree->bucket) { // internal
		parent = tree;
		if (tuple[tree->dimension] <= tree->discriminant) { // go left
			tree = tree->left;
		} else {
			tree = tree->right;
		}
	} // while internal
	// base case: a bucket
	if (tree->bucket->count < BUCKETSIZE) {
		tree->bucket->key[tree->bucket->count] = tuple;	
		tree->bucket->glyphs[tree->bucket->count] = theGlyph;	
		tree->bucket->count += 1;
	} else { // overflow; split bucket
		// fprintf(stderr, "splitting a bucket\n");
	// compute dimension
		int dimension =
			maxSpreadDimension(tree->bucket->key, tree->bucket->count);
	// compute discriminant index
		int partitionAt = median(
			tree->bucket->key,
			tree->bucket->glyphs,
			dimension,
			0,
			tree->bucket->count-1);
		// p_printBucket(tree->bucket, dimension, "after partition");
		// fprintf(stdout, "dimension %d, index %d\n",
		// 	dimension, partitionAt);
		if (partitionAt == -1) { // failure to find median; we discard insert
			p_printBucket(tree->bucket, dimension, "before split");
			return;
		}
	// 	bucket now becomes the internal node
		tree->dimension = dimension;
		tree->discriminant = tree->bucket->key[partitionAt][dimension];
		// bounds remain unchanged
	// create children (buckets) for new internal node
		tree->left = buildBucket(tree->bucket, 0, partitionAt, tree);
		tree->left->highBounds[tree->dimension] = tree->discriminant;
		// p_printBucket(tree->left->bucket, dimension, "new left bucket");
		tree->right = buildBucket(tree->bucket, partitionAt+1,
			tree->bucket->count-1, tree);
		tree->right->lowBounds[tree->dimension] =
			tree->discriminant;
		// p_printBucket(tree->right->bucket, dimension, "new right bucket");
		// p_printTree(tree, dimension, "after splitting");
	//  discard the old bucket
		free(tree->bucket);
		tree->bucket = 0;
	// insert the new tuple in the appropriate child
		p_kd_node_t *insertChild = (tuple[dimension] <= tree->discriminant)
			? tree->left : tree->right;
		insertChild->bucket->key[insertChild->bucket->count] = tuple;	
		insertChild->bucket->glyphs[insertChild->bucket->count] = theGlyph;	
		insertChild->bucket->count += 1;
		// p_printTree(tree, dimension, "after inserting");
	} // bucket overflow
} // p_insertTuple

void p_insertGlyph(const glyph_t *theGlyph) {
	p_insertTuple(layout, p_newTuple(theGlyph->top, theGlyph->left), theGlyph);
	p_insertTuple(layout, p_newTuple(theGlyph->top, theGlyph->right), theGlyph);
	p_insertTuple(layout, p_newTuple(theGlyph->bottom, theGlyph->left), theGlyph);
	p_insertTuple(layout, p_newTuple(theGlyph->bottom, theGlyph->right), theGlyph);
} // p_insertGlyph

float p_closestGlyph(const glyph_t *theGlyph, const glyph_t **closestGlyph) {
	float bestDistance = VERYBIG;
	float distance;
	p_bucket_t *bucket;
	int index;
	float tuple[2];
	tuple[0] = theGlyph->top; tuple[1] = theGlyph->left;
	bestDistance = p_closestMatch(layout, tuple, &bucket, &index, VERYBIG);
	if (index == -1) {
		fprintf(stdout, "this must be the first glyph; no neighbors\n");
		return BIGDIST;
	}
	*closestGlyph = bucket->glyphs[index];
	tuple[1] = theGlyph->right;
	distance = p_closestMatch(layout, tuple, &bucket, &index, VERYBIG);
	if (distance < bestDistance) {
		*closestGlyph = bucket->glyphs[index];
		bestDistance = distance;
	}
	tuple[0] = theGlyph->bottom;
	distance = p_closestMatch(layout, tuple, &bucket, &index, VERYBIG);
	if (distance < bestDistance) {
		*closestGlyph = bucket->glyphs[index];
		bestDistance = distance;
	}
	tuple[1] = theGlyph->left;
	distance = p_closestMatch(layout, tuple, &bucket, &index, VERYBIG);
	if (distance < bestDistance) {
		*closestGlyph = bucket->glyphs[index];
		bestDistance = distance;
	}
	fprintf(stdout, "closest to glyph at [%d, %d] is one at [%d, %d], distance %3.2f\n",
		theGlyph->top, theGlyph->left, (*closestGlyph)->top, (*closestGlyph)->left,
		bestDistance);
	return(bestDistance);
} // p_closestGlyph

// Find the bucket in which a given tuple resides.  Return both the
// bucket and the index within that bucket.  If the tuple is not in the
// tree, return the bucket it should go into, and the index where it
// should go.
void p_getBucket(p_tuple_t tuple, p_bucket_t **bucket, int *index) {
	p_kd_node_t *tree = layout;
	while (!(tree->bucket)) { // internal node
		if (tuple[tree->dimension] <= tree->discriminant) {
			tree = tree->left;
		} else {
			tree = tree->right;
		}
	}
	*bucket = tree->bucket;
	int count;
	for (count = 0; count < tree->bucket->count; count += 1) { // each tuple
		float dist = distSquared(tuple, (*bucket)->key[count]);
		if (dist == 0.0) break;
	} // each tuple
	*index = count;
} // p_getBucket

void p_freeTree(p_kd_node_t *tree) {
	if (!tree) return;
	free(tree->lowBounds);
	free(tree->highBounds);
	if (tree->bucket) { // bucket
		// fprintf(stdout, "freeing a bucket\n");
		int element;
		for (element = 0; element < tree->bucket->count; element += 1) {
			free(tree->bucket->key[element]);
			free((char *) tree->bucket->glyphs[element]);
		} // each element
		free(tree->bucket);
	} else { // internal node
		// fprintf(stdout, "freeing an internal node\n");
		p_freeTree(tree->left);
		p_freeTree(tree->right);
	}
	free(tree);
} // p_freeTree

p_kd_node_t *p_buildEmptyTree() {
	// initial tree is an empty bucket with infinite bounds.
	p_kd_node_t *answer;
	answer = (p_kd_node_t *) malloc(sizeof(p_kd_node_t));
	answer->lowBounds = (p_tuple_t) malloc(sizeof(float) * DIMS);
	answer->highBounds = (p_tuple_t) malloc(sizeof(float) * DIMS);
	int dimension;
	for (dimension = 0; dimension < DIMS; dimension += 1 ) {
		answer->lowBounds[dimension] = -VERYBIG;		
		answer->highBounds[dimension] = VERYBIG;		
	} // each dimension
	answer->dimension = 0;
	answer->bucket = (p_bucket_t *) malloc(sizeof(p_bucket_t));
	answer->bucket->count = 0;
	return(answer);
} // p_buildEmptyTree

void p_init() {
	if (layout != NULL) {
		p_freeTree(layout);
	}
	layout = p_buildEmptyTree();
} // p_init
