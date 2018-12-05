/*
gtkDisplay.c
reference: http://developer.gnome.org/doc/API/2.0/gtk/

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


// #include <fribidi/fribidi.h>
#include "fribidi/fribidi.h" // use local copy; not everyone has installed this
#include <iconv.h>
// #include "gtk/gtk.h"
#include <gtk/gtk.h>
#include "ocr.h"

// just for here
static GtkWidget *infoGuessValue = NULL; // GtkLabel
static GtkWidget *infoNewValue = NULL; // GtkEntry
static glyph_t *curGlyph;
static GtkObject *pscroll; // picture scroller
static GtkObject *oscroll; // ocr scroller
static float oScrollFraction = -1.0; // negative means not in use
GtkStyle *redStyle, *greenStyle, *blueStyle, *yellowStyle;

// called when the picWin is finally made visible
gboolean paintGlyphRectangles(GtkWidget *picWin, gpointer func_data) {
	redStyle = gtk_style_attach(redStyle, picWin->window);
	greenStyle = gtk_style_attach(greenStyle, picWin->window);
	blueStyle = gtk_style_attach(blueStyle, picWin->window);
	yellowStyle = gtk_style_attach(yellowStyle, picWin->window);
	GtkStyle *style = picWin->style;
	int requisitionWidth = picWin->requisition.width;
	int allocationWidth = picWin->allocation.width;
	int xoffset = (allocationWidth - requisitionWidth) / 2;
	// printf ("rw = %d, aw = %d; xoffset = %d\n", requisitionWidth,
	// allocationWidth, xoffset);
	lineHeaderList *curLine;
	for (curLine = lineHeaders->next; curLine; curLine=curLine->next) {
		glyph_t *aGlyph = curLine->line->glyphs->next;
		for (; aGlyph; aGlyph = aGlyph->next) { // a glyph
			// printf("distance seems to be %f\n", aGlyph->distance);
			bucket_t *bucket;
			int index;
			if (aGlyph->distance == -1) {
				aGlyph->distance =
					closestMatch(categorization, aGlyph->tuple, &bucket, &index,
					BIGDIST);
			}
			int width = aGlyph->right - aGlyph->left;
			if (aGlyph->distance > goodMatch) {
				// printf("distance is %f\n", aGlyph->distance);
				gtk_draw_box(
					aGlyph->distance > minMatch ? redStyle : greenStyle,
					picWin->window, GTK_STATE_NORMAL,
					GTK_SHADOW_OUT, aGlyph->left + xoffset, aGlyph->top-8,
					width+1, 5);
			}
			if (!useFlood && aGlyph->leftPath) {
				// show where leftPath applies in yellow
				gtk_draw_box(
					yellowStyle,
					picWin->window, GTK_STATE_NORMAL,
					GTK_SHADOW_OUT, aGlyph->left + xoffset, aGlyph->top-10,
					(width+1)/2, 5);
			}
			if (!useFlood && aGlyph->rightPath) {
				// show where rightPath applies in blue
				gtk_draw_box(
					blueStyle,
					picWin->window, GTK_STATE_NORMAL,
					GTK_SHADOW_OUT, aGlyph->left + xoffset + width/2, aGlyph->top-10,
					(width+1)/2, 5);
			}
			gtk_paint_shadow(
				style,
				picWin->window, GTK_STATE_NORMAL, GTK_SHADOW_IN, NULL,
				NULL, "ctk_curve", aGlyph->left + xoffset, aGlyph->top-1,
				width+1, aGlyph->bottom - aGlyph->top+2);
		} // each glyph
	} // each line
	// fprintf(stdout, "done with paint\n");
	return(0); // can propagate further
} // paintGlyphRectangles

gboolean acceptInformation(GtkWidget *viewport, GdkEventButton *event,
		gpointer func_data) {
	// fprintf(stdout, "x is %d, y is %d\n", (int) event->x, (int) event->y);
	// int requisitionWidth = viewport->requisition.width;
	// int allocationWidth = viewport->allocation.width;
	// int xoffset = (allocationWidth - requisitionWidth) / 2;
	glyph_t *theGlyph = glyphAtXY(event->x, event->y, 0);
	if (theGlyph) {
		curGlyph = theGlyph; // global
		gtk_entry_set_text((GtkEntry *)infoNewValue, "");
		// tuple_t tuple = newTuple();
		// fillTuple(curGlyph, tuple);
		bucket_t *bucket;
		int index;
		float distance =
			closestMatch(categorization, theGlyph->tuple, &bucket, &index, BIGDIST);
		//fprintf(stdout, theGlyph->tuple);
		char buf[BUFSIZ];
		sprintf(buf, "‪%s‬ (distance %3.2f)",
			index > -1 ? bucket->values[index] : "unknown value", distance);
		sprintf(buf + strlen(buf), " height %d, width %d at [x,y]=%d,%d",
			theGlyph->bottom - theGlyph->top,
			theGlyph->right - theGlyph->left,
			theGlyph->left, theGlyph->top
			);
		/* debugging output {
		fprintf(stderr, "glyph data [%d,%d] to [%d,%d]:\n\t",
			theGlyph->left, theGlyph->top, theGlyph->right, theGlyph->bottom);
		int dim;
		for (dim=0; dim < GRID*GRID+1; dim += 1) {
			fprintf(stderr, "%3.0f/", 1000*theGlyph->tuple[dim]);
		}
		fprintf(stderr, "\n");
		fprintf(stderr, "closest match data:\n\t");
		tuple_t theTuple = bucket->key[index];
		for (dim=0; dim < GRID*GRID+1; dim += 1) {
			fprintf(stderr, "%3.0f/", 1000*theTuple[dim]);
		}
		fprintf(stderr, "\n");
		end of debugging output } */
		if (index > -1 && distance > 0.0) {
			sprintf(buf + strlen(buf), " right-click to accept");
		}
		gtk_label_set_text((GtkLabel *)infoGuessValue, buf);
		if (event->button == 3 && index > -1 && distance > 0.0) {
			// accept this as a new value
			// fprintf(stdout, "button %d pressed\n", event->button);
			char *valueCopy = malloc(strlen(bucket->values[index]) + 1);
			strcpy(valueCopy, bucket->values[index]);
			setCategory(theGlyph, valueCopy);
			gtk_label_set_text((GtkLabel *) infoGuessValue, valueCopy);
			gtk_entry_set_text((GtkEntry *)infoNewValue, valueCopy);
		}
	} else {
		glyphAtXY(event->x, event->y, 0); // find out why no glyph there
		curGlyph = NULL;
		gtk_entry_set_text((GtkEntry *)infoNewValue, "");
		gtk_label_set_text((GtkLabel *)infoGuessValue,  "no glyph there");
		fprintf(stdout, "no glyph at (%d, %d)\n", (int) event->x, (int) event->y);
	}
	return(0); // can propagate further
} // acceptInformation

gboolean acceptValue(GtkEntry *newValueWidget, gpointer data) {
	const char *theValue = gtk_entry_get_text(newValueWidget);
	// fprintf(stdout, "activated; got %s\n", theValue);
	if (curGlyph) {
		char *valueCopy = malloc(strlen(theValue) + 1);
		strcpy(valueCopy, theValue);
		setCategory(curGlyph, valueCopy);
		gtk_label_set_text((GtkLabel *) infoGuessValue, valueCopy);
	}
	return(0);
} // acceptValue

FILE *tensorStream = NULL;

static void initTensorFile(){
        tensorStream = fopen(tensorFile, "r");
        if(tensorStream==NULL){
		perror("fopen");
		exit(EXIT_FAILURE);
        }
}

//This would be nice to have
//HARDCODE FILE
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
	return prediction;
}

// simple textual output
void showText() { // called from a button; ignore any parameters
	fprintf(stdout, "\ntext\n");
	lineHeaderList *curLine;
	for (curLine = lineHeaders->next; curLine; curLine=curLine->next) {
		glyph_t *glyphPtr;
		char buf[BUFSIZ];
		char *bufPtr = buf;
		int prevRight = 0; // where the previous character ended
		for (glyphPtr = curLine->line->glyphs->next; glyphPtr;
				glyphPtr = glyphPtr->next) {
			if (glyphPtr->left - prevRight > glyphWidth / 2) {
				bufPtr += sprintf(bufPtr, " ");
			}
			//RIGHT HERE
			bufPtr += sprintf(bufPtr, "%s", ocrValue(glyphPtr->tuple));
			//fprintf(stdout, "%d", glyphPtr->tuple);
			prevRight = glyphPtr->right;
		} // one glyph
		fprintf(stdout, "%s\n", buf);
	} // one line
} // showText

// simple textual output
void showTensorFlow() { // called from a button; ignore any parameters
	fprintf(stdout, "\ntensor text\n");
	lineHeaderList *curLine;
	for (curLine = lineHeaders->next; curLine; curLine=curLine->next) {
		glyph_t *glyphPtr;
		char buf[BUFSIZ];
		char *bufPtr = buf;
		int prevRight = 0; // where the previous character ended
		for (glyphPtr = curLine->line->glyphs->next; glyphPtr;
				glyphPtr = glyphPtr->next) {
			if (glyphPtr->left - prevRight > glyphWidth / 2) {
				bufPtr += sprintf(bufPtr, " ");
			}
			//RIGHT HERE
			bufPtr += sprintf(bufPtr, "%s", ocrValue(glyphPtr->tuple));
			//fprintf(stdout, "%d", glyphPtr->tuple);
			prevRight = glyphPtr->right;
		} // one glyph
		fprintf(stdout, "%s\n", buf);
	} // one line
} // showText

// collect the OCR values of the text, but not using more than lengthAvailable.
// Return how many characters were filled.
static int collectText(glyph_t *glyphPtr, char *buffer, int lengthAvailable) {
	int filled = 0;
	if (!glyphPtr) return 0;
	if (lengthAvailable < 5) return(0); // don't get too close to end
	if(!printTensorFlow)
		strncpy(buffer, ocrValue(glyphPtr->tuple), lengthAvailable-1);
	else
		strncpy(buffer, getTensorOcr(), lengthAvailable-1);
	lengthAvailable -= strlen(buffer);
	filled = strlen(buffer);
	if (glyphPtr->next &&
			glyphPtr->next->left - glyphPtr->right >
			spaceFraction*glyphWidth) { // wide space in text
		int spaces =
			MAX(1, (glyphPtr->next->left - glyphPtr->right)/glyphWidth - 1);
		while (spaces) {
			strcat(buffer + filled, " ");
			lengthAvailable -= 1;
			filled += 1;
			spaces -= 1;
		}
	} // wide space in text
	filled += collectText(glyphPtr->next, buffer + filled, lengthAvailable);
	lengthAvailable -= filled;
	return(filled);
} // collectText

// collect the OCR values of the text, but not using more than lengthAvailable.
// Return how many characters were filled.

/*
//static int collectTextTensor(glyph_t *glyphPtr, char *buffer, int lengthAvailable, FILE *tFile) {
static int collectTextTensor(glyph_t *glyphPtr, char *buffer, int lengthAvailable) {
	int filled = 0;
	if (!glyphPtr) return 0;
	if (lengthAvailable < 5) return(0); // don't get too close to end

	//Reading line by line from the TensorFlow input document
	//char *prediction = NULL;
	//size_t len=0;
	//ssize_t pread;
	//int length;
	//if((pread = getline(&prediction, &len, tFile)) != -1){
	//	length = strlen(prediction);
	//	if(prediction[length-1]=='\n')
	//		prediction[length-1]='\0';
	//}
	char *tensorOcrVal = getTensorOCR(tFile);

	strncpy(buffer, tensorOcrVal, lengthAvailable-1);
	lengthAvailable -= strlen(buffer);
	filled = strlen(buffer);

	if (glyphPtr->next &&
			glyphPtr->next->left - glyphPtr->right >
			spaceFraction*glyphWidth) { // wide space in text
		int spaces =
			MAX(1, (glyphPtr->next->left - glyphPtr->right)/glyphWidth - 1);
		while (spaces) {
			strcat(buffer + filled, " ");
			lengthAvailable -= 1;
			filled += 1;
			spaces -= 1;
		}
	} // wide space in text

	//filled += collectTextTensor(glyphPtr->next, buffer + filled, lengthAvailable, tFile);
	filled += collectTextTensor(glyphPtr->next, buffer + filled, lengthAvailable);
	lengthAvailable -= filled;


	return(filled);
} // collectText
*/

// these are all created together
GtkTextBuffer *textBuffer;

// main window; declared global so we can retain from page to page
GtkWidget *mainWindow = NULL;
GtkWidget *mainBox = NULL;
GtkWidget *picScroller = NULL;
GtkWidget *picViewport = NULL;
GtkLayout *infoLayout = NULL;
GtkWidget *textView = NULL;
GtkWidget *title = NULL;
GtkWidget *aligned = NULL;
static int firstDisplay = 1;  // reset to 0 after building the GUI

// graphical output using Gtk
void displayText(void *theButton, int *visual) { // from signal
// clear any previous text
	if ((int) (*visual)) {
		GtkTextIter startIter, endIter;
		gdouble myValue = -1.0, myUpper, myPageSize;
		g_object_get(oscroll,
			"value", &myValue, 
			"upper", &myUpper,
			"page-size", &myPageSize,
			NULL);
		oScrollFraction = myValue / (myUpper - myPageSize);
		gtk_text_buffer_get_start_iter(textBuffer, &startIter);
		gtk_text_buffer_get_end_iter(textBuffer, &endIter);
		gtk_text_buffer_delete(textBuffer, &startIter, &endIter);
	} // visual
// fill new text
	char lineBuf[BUFSIZ];
	lineHeaderList *curLine;
	int prevBottom = 0;

	//FILE *tensorStream = NULL;
	//Open TensorFlow predictions file, if neccessary
	if(printTensorFlow){
		initTensorFile();
	}
	//if(printTensorFlow){
	//	tensorStream = fopen(tensorFile, "r");
	//	if(tensorStream==NULL){
        //        	perror("fopen");
        //        	exit(EXIT_FAILURE);
        //       }
	//}

	for (curLine = lineHeaders->next; curLine; curLine=curLine->next) {
		int blankLines = 0;
		textLine *curText = curLine->line;
		if (prevBottom != 0) {
			blankLines = (curText->top - prevBottom) / (curText->bottom - curText->top);
			blankLines = MAX(blankLines, 0);
			if (blankLines > 2) blankLines = blankLines/2 + 1; // reduce multiple blanks
			// fprintf(stdout, "distance is %d; width is %d; blank lines=%d\n",
			// 	curText->top - prevBottom, curText->bottom - curText->top, blankLines);
		}
		prevBottom = curText->bottom;

		//int filled;
		//if(!printTensorFlow){
		int filled = collectText(curText->glyphs->next, lineBuf, BUFSIZ);
		//} else {
		//	filled = collectTextTensor(curText->glyphs->next, lineBuf, BUFSIZ, tensorStream);
		//
		//}

		if (0 && RTL != hasRTL(lineBuf)) { //  direction is wrong, do it again.
			// fprintf(stderr, "switching in line to %s\n",
			// 	RTL ? "LTR" : "RTL");
			RTL = 1 - RTL; // just temporarily 
			filled = collectText(curText->glyphs->next, lineBuf, BUFSIZ);
			RTL = 1 - RTL;
		}
		// fprintf(stderr, "RTL is %d\n", RTL);
		strcat(lineBuf + filled, "\n");
		// fprintf(stdout, "line %d of %d (length %d): %s",
		// 	lineIndex, numLines, filled, lineBuf);
	// 	adjust for indentation
		char spaces[BUFSIZ];
		memset(spaces, ' ', BUFSIZ);
		spaces[BUFSIZ-1] = 0;
		int indent = RTL
			? (rightMargin - curText->rightBorder) / glyphWidth
			: (curText->leftBorder - leftMargin) / glyphWidth;
		indent = MIN(indent, strlen(spaces));
		if ((int) (*visual)) {
			gtk_text_buffer_insert_at_cursor(textBuffer, spaces, indent);
		} else {
			spaces[indent] = 0;
			fprintf(stdout, "%s", spaces);
			spaces[indent] = ' ';
		}

	//  bidi algorithm 
		/* */
		// fprintf(stderr, "before: %s\n", lineBuf);
		// convert UTF8 to UCS4: lineBuf to UCS4Buf 
		FriBidiChar *UCS4Buf =
			(FriBidiChar *) calloc(filled+10, sizeof(FriBidiChar));
		int length = fribidi_utf8_to_unicode(lineBuf, filled+1, UCS4Buf);
		// apply bidi to UCS4Buf, putting result in newbuf
		FriBidiChar *newbuf = (FriBidiChar *)
			calloc(filled+10, sizeof(FriBidiChar));
		FriBidiCharType dirs[2] =
			{RTL ? FRIBIDI_TYPE_RTL : FRIBIDI_TYPE_LTR, 0};
		fribidi_set_reorder_nsm(1);
		(void) fribidi_log2vis(UCS4Buf, length, dirs, newbuf,
			NULL, NULL, NULL);
		// convert UCS4 to UTF8: newbuf to charbuf
		char *charbuf = malloc(BUFSIZ);
		fribidi_unicode_to_utf8(newbuf, filled+1, charbuf);
		// see what it looks like
		// fprintf(stderr, "\tafter: %s\n", charbuf);
		if (charbuf[strlen(charbuf)-1] != '\n') {
			// fprintf(stderr, "adding cr\n");
			strcat(charbuf, "\n");
		} else {
			// fprintf(stderr, "not adding to %s\n", charbuf);
		}
		strncpy(lineBuf, charbuf + (*charbuf == '\n' ? 1 : 0), filled+1);
		// clean up
		free(UCS4Buf);
		free(newbuf);
		free(charbuf);
		/* */
	//  output data itself
		// fprintf(stderr, "%d blank lines\n", blankLines);
		if ((int) (*visual)) {
			for (; blankLines; blankLines -= 1) {
				gtk_text_buffer_insert_at_cursor(textBuffer, "\n", 1);
			}
			gtk_text_buffer_insert_at_cursor(textBuffer, lineBuf, filled+1);
			// don't adjust the scroller, because it miscalculates until
			// redisplay happens.
		} else {
			for (; blankLines; blankLines -= 1) {
				fprintf(stdout, "\n");
			}
			//RIGHT HERE
			if(!doTensorFlow)
				fprintf(stdout, "%s", lineBuf);
			//fprintf(stdout, "%s", "TESTEST");
		}
	} // each line


	//Close TensorFlow predictions file if one was used
	if(printTensorFlow){
		fclose(tensorStream);
                exit(EXIT_SUCCESS);
	}

	if ((int) (*visual)) {
		gtk_widget_show_all(mainWindow);
	} else {
		if(!doTensorFlow)
			fprintf(stdout, "");
		fflush(stdout);
	}
} // displayText

void scrollChanged(GtkAdjustment *scrollAdjustment, gpointer dummy) {
	gdouble myValue, myUpper, myPageSize;
	gdouble otherUpper, otherPageSize;
//	if oScrollFraction is set (by displayText), believe it.
	if (oScrollFraction > 0.0) {
		g_object_get(oscroll,
			"value", &myValue, 
			"upper", &myUpper,
			"page-size", &myPageSize,
			NULL);
		g_object_set(oscroll,
			"value", (myUpper - myPageSize) * oScrollFraction,
			NULL);
		oScrollFraction = -1.0; // turn off this feature for now
	}
//  get the values of the moved scroller
	g_object_get(scrollAdjustment,
		"value", &myValue, 
		"upper", &myUpper,
		"page-size", &myPageSize,
		NULL);
	gdouble fraction = myValue / (myUpper - myPageSize);
	// fprintf(stdout, "scroll changed to %0.5f \n", fraction);
//  get relevant values of the other scroller
	GtkObject *otherScroller = 
		(scrollAdjustment == (GtkAdjustment *) oscroll) ? pscroll : oscroll;
	g_object_get(otherScroller, 
		"upper", &otherUpper,
		"page-size", &otherPageSize,
		NULL);
//  account for blank space at top and bottom of pic (pscroll)
	int blankTop, blankBottom;
	lineHeaderList *curLine;
	for (curLine = lineHeaders->next; curLine->next; curLine=curLine->next) {
		// march to end
	}
	blankTop = lineHeaders->next->line->top;
	blankBottom = curLine->line->bottom;
	// fprintf(stdout, "top is %d, bottom is %d\n", blankTop, blankBottom);
//  set value of the other scroller
	//	g_object_set(otherScroller,
	//		"value", (otherUpper - otherPageSize) * fraction,
	//		NULL);
	float otherFraction;
	if (scrollAdjustment == (GtkAdjustment *) oscroll) {
		// the other is the picture
		otherFraction = ((blankBottom - blankTop)*fraction + blankTop) /
			otherUpper;
	} else {
		otherFraction = (myUpper*fraction - blankTop) /
			(blankBottom - blankTop);
	}
	g_object_set(otherScroller,
		"value",
			(otherUpper - otherPageSize) * otherFraction,
		NULL);
} // scrollChanged

// declared outside so we can detect and remove previous references
GdkPixbuf *gdkPixbuf = NULL;

void doExit() {
	fprintf(stdout, "quit button pressed\n");
	exit(0);
} // doExit

void tensorFlow() {
	//Include things here
	exit(0);
} // tensorFlow


int redo;

void redoColumn() {
	fprintf(stdout, "redoing pressed\n");
	redo = 1;
	gtk_main_quit();
} // doExit

void GUI(int col) {
// gtk initialization
	int argc = 0;
	char **argv = 0;
	gtk_init(&argc, &argv);
	int visual = 1;
	gdouble zero = 0.0;
//  main window
	if (firstDisplay) {
		mainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		mainBox = gtk_vbox_new(FALSE, 1);
		gtk_container_add(GTK_CONTAINER(mainWindow), mainBox);
// widget to display the picture of the document: picWin
	}
	GtkWidget *picImage; // image widget: picImage
	if (1 || col == columns-1) {
		int goodWidth = (width > GOODWIDTH) ? GOODWIDTH : width + 6;
		int goodHeight = (height > GOODHEIGHT) ? GOODHEIGHT : height + 6;
		picImage = NULL;
	// #define FROMFILE 1
	#ifdef FROMFILE
			char *fileName = malloc(strlen(fileBase) + 6);
			strcpy(fileName, fileBase);
			strcat(fileName, ".tiff");
			picImage = gtk_image_new_from_file(fileName);
	#else
			// Draw directly from raster so we can get multiple pages
			if (gdkPixbuf) {
				gdk_pixbuf_unref(gdkPixbuf);
			}
			gdkPixbuf = gdk_pixbuf_new_from_data((const guchar *) raster,
				GDK_COLORSPACE_RGB, true, 8, width, height, width*4, NULL, NULL);
			picImage = gtk_image_new_from_pixbuf(gdkPixbuf);
	#endif
		// viewport widget: picViewport
			if (firstDisplay) {
				picViewport = gtk_viewport_new(
					(GtkAdjustment *)
						gtk_adjustment_new(0, 0, width, width/100, width/4, width),
					(GtkAdjustment *)
						gtk_adjustment_new(0, 0, height, height/100, height/4, height));
				gtk_widget_set_size_request(picViewport,
					goodWidth < MINWIDTH ? MINWIDTH : goodWidth, goodHeight);
				gtk_widget_set_events(picViewport, GDK_BUTTON_PRESS_MASK);
				g_signal_connect(GTK_OBJECT(picViewport),
					"button-press-event",
					G_CALLBACK(acceptInformation), NULL);
				redStyle = gtk_style_copy(picViewport->style);
				GdkColor red;
				if (!gdk_color_parse("red", &red)) {
					fprintf(stderr, "did not find red\n");
				}
				redStyle->bg[0] = red;
				greenStyle = gtk_style_copy(picViewport->style);
				GdkColor green;
				if (!gdk_color_parse("green", &green)) {
					fprintf(stderr, "did not find green\n");
				}
				greenStyle->bg[0] = green;
				blueStyle = gtk_style_copy(picViewport->style);
				GdkColor blue;
				if (!gdk_color_parse("blue", &blue)) {
					fprintf(stderr, "did not find blue\n");
				}
				blueStyle->bg[0] = blue;
				yellowStyle = gtk_style_copy(picViewport->style);
				GdkColor yellow;
				if (!gdk_color_parse("yellow", &yellow)) {
					fprintf(stderr, "did not find yellow\n");
				}
				yellowStyle->bg[0] = yellow;
			} // build picViewport
		// scrollable container: picScroller
			if (firstDisplay) {
				pscroll = gtk_adjustment_new(0,0,0,0,0,0);
				picScroller =
					gtk_scrolled_window_new(NULL, (GtkAdjustment *) pscroll);
				g_signal_connect(GTK_OBJECT(pscroll),
					"value-changed",
					G_CALLBACK(scrollChanged), NULL);
			} // build picScroller
		// assemble the pieces
			if (firstDisplay) {
				gtk_container_add(GTK_CONTAINER(mainBox), picScroller);
				gtk_container_add(GTK_CONTAINER(picScroller), picViewport);
				aligned = gtk_alignment_new(0, 0, 0, 0);
					// force left if too small to fit
			}
			gtk_container_add(GTK_CONTAINER(aligned), picImage);
			if (firstDisplay) {
				gtk_container_add(GTK_CONTAINER(picViewport), aligned);
			}
			// gtk_container_add(GTK_CONTAINER(picViewport), picImage); // old
		// paint the rectangles after image widget is realized.
			g_signal_connect_after(GTK_OBJECT(picImage), "expose_event",
				G_CALLBACK(paintGlyphRectangles), NULL);
	// information layout
		if (infoLayout == NULL) {
			infoLayout = (GtkLayout *) gtk_layout_new(NULL, NULL);
			gtk_container_add(GTK_CONTAINER(mainBox), (GtkWidget *) infoLayout);
			gtk_widget_set_size_request((GtkWidget *)infoLayout, goodWidth, 180);
			title = gtk_label_new("");
			gtk_layout_put(infoLayout, title, 0, 0);
			infoGuessValue = (GtkWidget *) gtk_label_new("No guess");
			gtk_widget_modify_font(infoGuessValue,
				pango_font_description_from_string("FreeSans 16")); 
			gtk_layout_put(infoLayout, infoGuessValue, 0, 30);
			infoNewValue = (GtkWidget *) gtk_entry_new();
			gtk_layout_put(infoLayout, infoNewValue, 0, 60);
			g_signal_connect(GTK_OBJECT(infoNewValue), "activate",
				G_CALLBACK(acceptValue), NULL);
			gtk_widget_grab_focus(infoNewValue);
		} // establish infoLayout
		char buf[BUFSIZ];
		sprintf(buf, "image %s; font %s\n", fileBase, fontFile);
		gtk_label_set_text((GtkLabel *)title, buf);
	//  buttons
		if (firstDisplay) {
			GtkWidget *writeOut = gtk_button_new_with_label("Write font info");
			gtk_layout_put(infoLayout, writeOut, 0, 90);
			g_signal_connect(GTK_OBJECT(writeOut), "clicked",
				G_CALLBACK(writeTuples), NULL);
			GtkWidget *readIn = gtk_button_new_with_label("Read font info");
			gtk_layout_put(infoLayout, readIn, 0, 120);
			g_signal_connect(GTK_OBJECT(readIn), "clicked",
				G_CALLBACK(readTuples), NULL);
			GtkWidget *showTextButton = gtk_button_new_with_label("Show OCR text");
			gtk_layout_put(infoLayout, showTextButton, 0, 150);
			g_signal_connect(GTK_OBJECT(showTextButton), "clicked",
				G_CALLBACK(displayText), &visual);
			GtkWidget *quitButton = gtk_button_new_with_label("Quit");
			gtk_layout_put(infoLayout, quitButton, 250, 150);
			g_signal_connect(GTK_OBJECT(quitButton), "clicked",
				G_CALLBACK(doExit), NULL);
			char *pageLabel = (columns > 1 ? "Next column" : "Next page");
			GtkWidget *nextButton = gtk_button_new_with_label(pageLabel);
			gtk_layout_put(infoLayout, nextButton, 250, 120);
			g_signal_connect(GTK_OBJECT(nextButton), "clicked",
				G_CALLBACK(gtk_main_quit), NULL);
			char *redoLabel = (columns > 1 ? "Repeat" : "Redo column");
			GtkWidget *redoButton = gtk_button_new_with_label(redoLabel);
			gtk_layout_put(infoLayout, redoButton, 500, 120);
			g_signal_connect(GTK_OBJECT(redoButton), "clicked",
				G_CALLBACK(redoColumn), NULL);
			/*
			GtkWidget *testButton = gtk_button_new_with_label("test");
			gtk_layout_put(infoLayout, testButton, 250, 120);
			g_signal_connect(GTK_OBJECT(testButton), "clicked",
				G_CALLBACK(simpleTest), NULL);
			*/
			GtkWidget *writeTrainingButton =
				gtk_button_new_with_label("write training");
			gtk_layout_put(infoLayout, writeTrainingButton, 250, 90);
			g_signal_connect(GTK_OBJECT(writeTrainingButton), "clicked",
				G_CALLBACK(writeTraining), NULL);
			GtkWidget *readTrainingButton =
				gtk_button_new_with_label("read training");
			gtk_layout_put(infoLayout, readTrainingButton, 250, 60);
			g_signal_connect(GTK_OBJECT(readTrainingButton), "clicked",
				G_CALLBACK(readTraining), NULL);
			GtkWidget *readTemplateButton =
				gtk_button_new_with_label("read template");
			gtk_layout_put(infoLayout, readTemplateButton, 500, 60);
			g_signal_connect(GTK_OBJECT(readTemplateButton), "clicked",
				G_CALLBACK(readTemplate), NULL);
			GtkWidget *writeTemplateButton =
				gtk_button_new_with_label("write template");
			gtk_layout_put(infoLayout, writeTemplateButton, 500, 90);
			g_signal_connect(GTK_OBJECT(writeTemplateButton), "clicked",
				G_CALLBACK(writeTemplate), NULL);
			GtkWidget *tensorFlowButton =
				gtk_button_new_with_label("TensorFlow");
			gtk_layout_put(infoLayout, tensorFlowButton, 500, 150);
			g_signal_connect(GTK_OBJECT(tensorFlowButton), "clicked",
				G_CALLBACK(showTensorFlow), NULL);
		} // buttons
		// ocr-processed text: textView
		if (firstDisplay) {
			textView = gtk_text_view_new();
			textBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));
			gtk_widget_modify_font(textView,
				pango_font_description_from_string("FreeSans 16")); 
			oscroll = gtk_adjustment_new(0,0,0,0,0,0);
			GtkWidget *textScroller =
				gtk_scrolled_window_new(NULL, (GtkAdjustment *) oscroll);
			g_signal_connect(GTK_OBJECT(oscroll),
				"value-changed",
				G_CALLBACK(scrollChanged), NULL);
			gtk_widget_set_size_request((GtkWidget *)textView, goodWidth, 200);
			gtk_container_add(GTK_CONTAINER(mainBox), textScroller);
			gtk_container_add(GTK_CONTAINER(textScroller), textView);
		}
		// gtk main loop
		firstDisplay = 0;
		displayText(NULL, &visual);
		gtk_widget_show_all(mainWindow);
		// gtk_container_remove(GTK_CONTAINER(picViewport), picImage);
	} else { // subsequent column
		displayText(NULL, &visual);
	}
	gtk_widget_grab_focus(infoNewValue);
	gtk_main();
	g_object_set(oscroll, "value", zero, NULL);
	if (1 || col == 0) {
		gtk_widget_destroy(picImage);
	}
	// gtk_widget_destroy(mainBox);
} // GUI

