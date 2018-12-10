/*
kd.c

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


#include <math.h>
#include "ocr.h"
#include <stdio.h>
#include <unistd.h>

//int count = 0;

kd_node_t *categorization; // the root
int RTL;
FILE *tensorStream = NULL;

static float distSquared(tuple_t tuple1, tuple_t tuple2) {
	int dimension;
	float answer = 0.0;
	int noLastDim = (ignoreVertical ? 1 : 0);
	for (dimension = 0; dimension < TUPLELENGTH - noLastDim; dimension += 1) {
		// fprintf(stderr, "\t\tdim %d: %f vs %f\n",
		// 	dimension, tuple1[dimension], tuple2[dimension]);
		float diff = tuple1[dimension] - tuple2[dimension];
		answer += diff * diff;
	} // each dimension
	return answer;
} // distSquared

static int ballWithinBounds(tuple_t probe, kd_node_t *treeNode,
		float trueDist) {
	int dimension;
	// fprintf(stdout, "\tball within bounds test, distance is %7.3f\n", trueDist);
	for (dimension = 0; dimension < TUPLELENGTH; dimension += 1) {
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
static int ballOverlapBounds(tuple_t probe, kd_node_t *treeNode,
		float trueDist) {
	int dimension;
	tuple_t closestCorner = newTuple();
	for (dimension = 0; dimension < TUPLELENGTH; dimension += 1) {
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
float closestMatch(kd_node_t *tree, tuple_t probe, bucket_t **bucket,
		int *index, float betterThan) {
	int closestIndex = -1;
	// fprintf(stdout, "\nstarting closestMatch test\n");
// initial dive to a bucket
	kd_node_t *stack[MAXTREEDEPTH];
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
	// fprintf(stderr, "closestMatch: bucket has %d elements\n",
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
	// fprintf(stderr, "closestMatch: radius %f\n", radius);
	while (stackSize) { // move back up the tree if necessary
		// fprintf(stdout,
		// 	"\tstack depth %d; checking ball within bounds; radius is %6.3f\n",
		// 	stackSize, radius);
		if (ballWithinBounds(probe, tree, radius)) {
			// fprintf(stdout, "\tsucceeded\n");
			break;
		}
		kd_node_t *parent = stack[--stackSize];
		// fprintf(stdout, "\tfailed; my sibling is on the %s\n",
		// 	(tree == parent->left) ? "right" : "left");
		kd_node_t *sibling =
			(tree == parent->left) ? parent->right : parent->left;
		if (ballOverlapBounds(probe, sibling, radius)) { // search sibling
			// fprintf(stdout, "\tsibling is worth checking\n");
			bucket_t *siblingBucket;
			int siblingIndex;
			float siblingDist = closestMatch(sibling, probe, &siblingBucket,
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
} // closestMatch

float ocrDistance2(tuple_t tuple) { // returns square of distance
	bucket_t *bucket;
	int index;
	closestMatch(categorization, tuple, &bucket, &index, BIGDIST);
	if (index == -1) return(BIGDIST);
	return (distSquared(tuple, bucket->key[index]));
} // ocrDistance

static void initTensorFile(){
        //fprintf(stderr, "MEOWWWWW initTensorFile\n");
	//fprintf(stderr, "MEOWWWWW3\n");
        tensorStream = fopen(tensorFile, "r");
        if(tensorStream==NULL){
                perror("fopen");
                exit(EXIT_FAILURE);
        }
}

//Find next character from TensorFlow predicted ocr output file, one new value on each line
static char* getTensorOcr(){
        char* prediction = NULL;
        size_t len=0;
        ssize_t pread;
        int length;
        if((pread = getline(&prediction, &len, tensorStream)) != -1){
                length = strlen(prediction);
                if(prediction[length-1]=='\n')
                        prediction[length-1]='\0';
        }
        //fprintf(stderr, "MEOWWWWW getTensorOcr\n");
        //fprintf(stderr, "Prediction: %s", prediction);
        return prediction;
}



const char *ocrValue(tuple_t tuple) {

        bucket_t *bucket;
        int index;

	if(!printTensorFlow)
        	closestMatch(categorization, tuple, &bucket, &index, BIGDIST); //where nearest neighbor makes the prediction

	//fprintf(stderr, "MEOWWWWW4\n");
	if(printTensorFlow && tensorStream==NULL)
		initTensorFile();

	if(printTensorFlow)
		return getTensorOcr();

        if(doTensorFlow)
        {
                //Print each tuple to stdout together with the char predicted by Batch Mode
                for(int i=0; i<27; i++)
                        fprintf(stdout, "%f, %s", tuple[i], " ");
                if (distSquared(tuple, bucket->key[index]) <= minMatch*minMatch)
                        fprintf(stdout, "%s", bucket->values[index], " ");
                else
                        fprintf(stdout, "%s", "XX", " ");	//unknown batch char marked as XX
                fprintf(stdout, "%s", "\n");
        }


        if (index == -1) return("·"); // empty tree

	if (distSquared(tuple, bucket->key[index]) <= minMatch*minMatch) {
                //printf("%s", bucket->values[index]);
                return(bucket->values[index]);
        } else {
                return(OCRFAILS);
        }

        //count = count+1;

        //fclose(fp);

} // ocrValue

int maxSpreadDimension(tuple_t *key, int count) {
	float maxSpread = 0;
	int answerDimension = 0;
	tuple_t mins = newTuple();
	tuple_t maxes = newTuple();
	if (count == 0) {
		fprintf(stderr, "Cannot compute dimensions of an empty list\n");
		exit(1);
	}
	memcpy(mins, key[0], sizeof(float) * TUPLELENGTH);
	memcpy(maxes, key[0], sizeof(float) * TUPLELENGTH);
	int tupleIndex;
	int dimension;
	for (tupleIndex = 1; tupleIndex < count; tupleIndex += 1) {
		tuple_t tuple = key[tupleIndex];
		for (dimension = 0; dimension < TUPLELENGTH; dimension += 1) {
			mins[dimension] = MIN(mins[dimension], tuple[dimension]);
			maxes[dimension] = MAX(maxes[dimension], tuple[dimension]);
		}
	} // each tuple
	for (dimension = 0; dimension < TUPLELENGTH; dimension += 1) {
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
		tuple_t tmpContent = key[(index1)]; \
		key[(index1)] = key[(index2)]; \
		key[(index2)] = tmpContent; \
		const char *tmpValue = values[(index1)]; \
		values[(index1)] = values[(index2)]; \
		values[(index2)] = tmpValue; \
	} while (0)

// partition the bucket between lowIndex and highIndex (inclusive) with respect
// to the given dimension, returning the pivot index.  Use Lomuto's algorithm
// (Jon Bentley (2000, p 117))
static int partition(tuple_t *key, const char **values, int dimension,
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
static void quickSort(tuple_t *key, const char **values,
		int dimension, int lowIndex, int highIndex) {
	if (highIndex - lowIndex <= 0) return; // base case
	int partitionAt = partition(key, values, dimension, lowIndex,
		highIndex);
	quickSort(key, values, dimension, lowIndex, partitionAt - 1);
	quickSort(key, values, dimension, partitionAt + 1, highIndex);
} // quickSort

// returns -1 if all values are the same, so there is no median.
static int median(tuple_t *key, const char **values,
		int dimension, int lowIndex, int highIndex) {
	quickSort(key, values, dimension, lowIndex, highIndex);
	int answer = (lowIndex + highIndex) / 2;
	if (0) { // debugging only
		int index;
		fprintf(stdout, "in dimension %d over [%d,%d]: ", dimension, lowIndex,
			highIndex);
		for (index = lowIndex; index <= highIndex; index += 1) {
			fprintf(stdout, "%d:%7.4f[%s] ", index, key[index][dimension],
				values[index]);
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
kd_node_t *buildBucket(bucket_t *oldBucket, int startIndex, int endIndex,
		kd_node_t *parent) {
	// fprintf(stdout, "building a bucket from %d through %d\n",
	// 	startIndex, endIndex);
	kd_node_t *answer = (kd_node_t *)malloc(sizeof(kd_node_t));
	answer->lowBounds = copyTuple(parent->lowBounds);
	answer->highBounds = copyTuple(parent->highBounds);
	answer->bucket = (bucket_t *) malloc(sizeof(bucket_t));
	answer->bucket->count = endIndex - startIndex + 1;
	if (!oldBucket) return(answer); // empty input, empty copy
	// fprintf(stdout, "copying key\n");
	memcpy(answer->bucket->key, oldBucket->key+startIndex,
		answer->bucket->count * sizeof(float *));
	memcpy(answer->bucket->values, oldBucket->values+startIndex,
		answer->bucket->count * sizeof(char *));
	return(answer);
} // buildBucket

// debugging support
void printTuple(tuple_t tuple, int specialDimension) {
	int dimension;
	for (dimension = 0; dimension< TUPLELENGTH; dimension+=1 ) {
		if (dimension == specialDimension) {
			fprintf(stdout, "(%6.3f) ", tuple[dimension]);
		} else {
			fprintf(stdout, "%6.3f ", tuple[dimension]);
		}
		// if (dimension >= 3) break; // debug
	} // each dimension
	fprintf(stdout, "\n");
} // printTuple

void printBucket(bucket_t *bucket, int dimension, const char *message) {
	if (strlen(message)) fprintf(stdout, "%s ", message);
	fprintf(stdout, "bucket ");
	int elementIndex;
	for (elementIndex = 0; elementIndex < bucket->count; elementIndex += 1) {
		fprintf(stdout, "[%s] ", bucket->values[elementIndex]);
		printTuple(bucket->key[elementIndex], dimension);
	} // each element
	fprintf(stdout, "\n");
} // printBucket

void printTree(kd_node_t *tree, int dimension, const char *message, 
		int indent) {
	fprintf(stdout, "%s ", message);
	int index;
	for (index = indent; index; index -= 1) fprintf(stdout, "  ");
	if (tree->bucket) {
		printBucket(tree->bucket, dimension, "");
	} else {
		// fprintf(stdout, "my low bounds are ");
		// printTuple(tree->lowBounds, -1);
		// fprintf(stdout, "my high bounds are ");
		// printTuple(tree->highBounds, -1);
		fprintf(stdout, "dimension %d, discriminant %6.3f\n", tree->dimension,
			tree->discriminant);
		printTree(tree->left, dimension, "", indent+1);
		printTree(tree->right, dimension, "", indent+1);
	}
} // printTree

void insertTuple(kd_node_t *tree, tuple_t tuple, char *values) {
	// kd_node_t *parent = 0;
	while (!tree->bucket) { // internal
		// parent = tree;
		if (tuple[tree->dimension] <= tree->discriminant) { // go left
			tree = tree->left;
		} else {
			tree = tree->right;
		}
	} // while internal
	// base case: a bucket
	if (tree->bucket->count < BUCKETSIZE) {
		tree->bucket->key[tree->bucket->count] = tuple;	
		tree->bucket->values[tree->bucket->count] = values;	
		tree->bucket->count += 1;
	} else { // overflow; split bucket
		// fprintf(stderr, "splitting a bucket\n");
	// compute dimension
		int dimension =
			maxSpreadDimension(tree->bucket->key, tree->bucket->count);
	// compute discriminant index
		int partitionAt = median(
			tree->bucket->key,
			tree->bucket->values,
			dimension,
			0,
			tree->bucket->count-1);
		// printBucket(tree->bucket, dimension, "after partition");
		// fprintf(stdout, "dimension %d, index %d\n",
		// 	dimension, partitionAt);
		if (partitionAt == -1) { // failure to find median; we discard insert
			printBucket(tree->bucket, dimension, "before split");
			return;
		}
	// 	bucket now becomes the internal node
		tree->dimension = dimension;
		tree->discriminant = tree->bucket->key[partitionAt][dimension];
		// bounds remain unchanged
	// create children (buckets) for new internal node
		tree->left = buildBucket(tree->bucket, 0, partitionAt, tree);
		tree->left->highBounds[tree->dimension] = tree->discriminant;
		// printBucket(tree->left->bucket, dimension, "new left bucket");
		tree->right = buildBucket(tree->bucket, partitionAt+1,
			tree->bucket->count-1, tree);
		tree->right->lowBounds[tree->dimension] =
			tree->discriminant;
		// printBucket(tree->right->bucket, dimension, "new right bucket");
		// printTree(tree, dimension, "after splitting");
	//  discard the old bucket
		free(tree->bucket);
		tree->bucket = 0;
	// insert the new tuple in the appropriate child
		kd_node_t *insertChild = (tuple[dimension] <= tree->discriminant)
			? tree->left : tree->right;
		insertChild->bucket->key[insertChild->bucket->count] = tuple;	
		insertChild->bucket->values[insertChild->bucket->count] = values;	
		insertChild->bucket->count += 1;
		// printTree(tree, dimension, "after inserting");
	} // bucket overflow
} // insertTuple

// Find the bucket in which a given tuple resides.  Return both the
// bucket and the index within that bucket.  If the tuple is not in the
// tree, return the bucket it should go into, and the index where it
// should go.
void getBucket(tuple_t tuple, bucket_t **bucket, int *index) {
	kd_node_t *tree = categorization;
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
} // getBucket

static void outTree(kd_node_t *tree, FILE *outFile) {
	if (tree->bucket) {
		int count;
		for (count = 0; count < tree->bucket->count; count += 1) { // each tuple
			tuple_t tuple = tree->bucket->key[count];
			int index;
			for (index = 0; index < TUPLELENGTH; index += 1) {
				float toPrint = tuple[index];
				if (index >= (GRID*GRID)) toPrint /= 3; // remove normalization
				fprintf(outFile, PRECISION, toPrint);
				fprintf(outFile, " ");
			}
			fprintf(outFile, "%s\n", tree->bucket->values[count]);
		} // each tuple
	} else { // internal node
		outTree(tree->left, outFile);
		if (tree->right != tree->left) outTree(tree->right, outFile);
	}
} // outTree

void writeTuples() {
	FILE *outFile = fopen(fontFile, "w");
	if (!outFile) {
		perror(fontFile);
		exit(1);
	}
	outTree(categorization, outFile);
	fclose(outFile);
} // writeTuples

extern long int fribidi_get_type_internal(int character);
#define FRIBIDI_MASK_RTL	0x00000001L	/* Is right to left */
#define FRIBIDI_MASK_NEUTRAL	0x00000040L	/* Is neutral */
int fribidi_utf8_to_unicode (const char *s, int len, int *us);

int hasRTL (const char *data) {
	int ustring[BUFSIZ];
	int ulength = fribidi_utf8_to_unicode(data, strlen(data), ustring);
	int index;
	for (index = 0; index < ulength; index += 1) {
		if (fribidi_get_type_internal(ustring[index]) & FRIBIDI_MASK_RTL) {
			// fprintf(stderr, "I see RTL at index %d / %d\n", index, ulength);
			return(true);
		}
	} // each unicode character
	return(false);
} // hasRTL

// given numEntries key and values (can be more than BUCKETSIZE of them),
// and given a personal copy of lowBounds and highBounds, which we may modify,
// return a new kd_node_t as deep as necessary to represent those entries.
// Recursive.
static kd_node_t *buildTreeFromData(tuple_t *key, const char **values,
		int numEntries, tuple_t lowBounds, tuple_t highBounds) {
	kd_node_t *answer = malloc(sizeof(kd_node_t));
	answer->lowBounds = lowBounds;
	answer->highBounds = highBounds;
	if (numEntries <= BUCKETSIZE) { // build a bucket
		answer->bucket = (bucket_t *) malloc(sizeof(bucket_t));
		memcpy(answer->bucket->key, key,
			numEntries * sizeof(float *));
		memcpy(answer->bucket->values, values,
			numEntries * sizeof(char *));
		answer->bucket->count = numEntries;
	} else { // build internal node
		answer->bucket = 0; // not a bucket
	// dimension and discriminant
		answer->dimension = maxSpreadDimension(key, numEntries);
		int partitionAt = median(
			key,
			values,
			answer->dimension,
			0,
			numEntries-1);
		// move to last identical entry
		answer->discriminant = key[partitionAt][answer->dimension];
	// new internal node
		int leftSize = partitionAt + 1;
	// children
		// left child
		tuple_t childLowBounds, childHighBounds;
		childLowBounds = copyTuple(lowBounds);
		childHighBounds = copyTuple(highBounds);
		childHighBounds[answer->dimension] = answer->discriminant;
		answer->left = buildTreeFromData(key, values,
			leftSize, childLowBounds, childHighBounds);
		// right child
		childLowBounds = copyTuple(lowBounds);
		childHighBounds = copyTuple(highBounds);
		childLowBounds[answer->dimension] = answer->discriminant;
		answer->right = buildTreeFromData(key + leftSize,
			values + leftSize, numEntries - leftSize,
			childLowBounds, childHighBounds);
	} // build internal node
	return(answer);
} // buildTreeFromData

#include <sys/stat.h>

void freeTree(kd_node_t *tree) {
	if (!tree) return;
	free(tree->lowBounds);
	free(tree->highBounds);
	if (tree->bucket) { // bucket
		// fprintf(stdout, "freeing a bucket\n");
		int element;
		for (element = 0; element < tree->bucket->count; element += 1) {
			free(tree->bucket->key[element]);
			free((char *) tree->bucket->values[element]);
		} // each element
		free(tree->bucket);
	} else { // internal node
		// fprintf(stdout, "freeing an internal node\n");
		freeTree(tree->left);
		freeTree(tree->right);
	}
	free(tree);
} // freeTree

void normalizeTuple(tuple_t tuple) {
	// increase the strength of the last elements
	tuple[GRID*GRID] *= 3;
	tuple[GRID*GRID+1] *= 3;
} // normalizeTuple

void readTuples() {
// open the file and get its length
	FILE *inFile = fopen(fontFile, "r");
	if (!inFile) {
		perror(fontFile);
		exit(1);
	}
	struct stat statBuf;
	stat(fontFile, &statBuf);
	int fileSize = statBuf.st_size;
// see how many entries we expect
	char buf[BUFSIZ];
	fgets(buf, BUFSIZ, inFile);
	int numEntries = fileSize / strlen(buf); // first estimate
	numEntries = 1.2 * numEntries; // a bit of slack
	// fprintf(stdout, "the file is %d bytes, "
	// 	"and each entry is about %d bytes, "
	// 	"so I expect no more than %d entries\n",
	// 	fileSize, strlen(buf), numEntries);
//  allocate space for the key and values
	tuple_t *key = malloc(sizeof(tuple_t) * numEntries);
	const char **values = malloc(sizeof(char *) * numEntries);
//  read all the entries
	fseek(inFile, 0, SEEK_SET); // back to the start
	int entryIndex = 0;
	while (!feof(inFile)) { // one entry (one line of input)
	// the content
		tuple_t tuple = newTuple();
		int tupleIndex;
		for (tupleIndex = 0; tupleIndex < TUPLELENGTH; tupleIndex += 1) {
			if (fscanf(inFile, " %f", &tuple[tupleIndex]) == EOF) {
				// fprintf(stdout, "from middle\n");
				free(tuple);
				goto out;
			}
		}
		// if (ignoreVertical) tuple[TUPLELENGTH-1] = 0.0;
		if (entryIndex >= numEntries) {
			fprintf(stderr, "I estimated %d entries, but there are more\n",
				numEntries);
			exit(1);
		}
		normalizeTuple(tuple);
		key[entryIndex] = tuple;
	// the values
		fscanf(inFile, " %s", buf);
		char *value = malloc(sizeof(buf) + 1);
		strcpy(value, buf);
		values[entryIndex] = value;
	// increment
		entryIndex += 1;
	} // each entry
	out:
	fclose(inFile);
	numEntries = entryIndex;
// compute RTL
	RTL = false;
	for (entryIndex = 0; entryIndex < 1000; entryIndex += 1) {
		if (entryIndex >= numEntries) break;
		// fprintf(stderr, "looking at %s\n", values[entryIndex]);
		if (hasRTL(values[entryIndex])) {
			RTL = true; // one is enough to convince us
			fprintf(stderr, "setting RTL\n");
			break;
		}
	} // for first 10 entries
	// fprintf(stderr, "The text is %sRTL\n", RTL ? "" : "not ");
// build the tree
	tuple_t lowBounds = newTuple();
	tuple_t highBounds = newTuple();
	int dimension;
	for (dimension = 0; dimension < TUPLELENGTH; dimension+=1 ) {
		lowBounds[dimension] = -INFTY;		
		highBounds[dimension] = INFTY;		
	} // each dimension
	if (categorization) freeTree(categorization);
	categorization = buildTreeFromData(key, values, numEntries, lowBounds,
		highBounds);
	// printTree(categorization, -1, "tree I just read in", 0);
} // readTuples

kd_node_t *buildEmptyTree() {
	// initial tree is an empty bucket with infinite bounds.
	kd_node_t *answer;
	answer = (kd_node_t *) malloc(sizeof(kd_node_t));
	answer->lowBounds = newTuple();
	answer->highBounds = newTuple();
	int dimension;
	for (dimension = 0; dimension < TUPLELENGTH; dimension += 1 ) {
		answer->lowBounds[dimension] = -INFTY;		
		answer->highBounds[dimension] = INFTY;		
	} // each dimension
	answer->dimension = 0;
	answer->bucket = (bucket_t *) malloc(sizeof(bucket_t));
	answer->bucket->count = 0;
	return(answer);
} // buildEmptyTree
