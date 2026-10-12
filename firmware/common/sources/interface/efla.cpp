// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      efla.cpp
//      Authors :   Po-Han Lin (http://www.edepot.com)
//      Date :      30th December 2023
//      Reviewed :  No
//      Purpose :   Extremely Fast Line Algorithm
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"

// ***************************************************************************************
//
//							Po-Han Lin's fast line algorithm
//
// ***************************************************************************************

static const char *_author = "Po-Han Lin (http://www.edepot.com)";

//		T-132 (0.16.91) : a line with an end off the screen used to be dropped altogether (5,2 from (-10,50)
//		to (100,50) drew nothing ; the rows of a filled ellipse that overflows vanished). It is now clipped
//		to the screen (Cohen-Sutherland) and the visible part drawn. Integer intersections : the clipped
//		ends lie on the ideal line within one pixel. Test API clipimg.
static int _GFXOutCode(struct GraphicsMode *gMode,int x,int y) {
	return (x < 0 ? 1 : 0) | (x >= gMode->xGSize ? 2 : 0) | (y < 0 ? 4 : 0) | (y >= gMode->yGSize ? 8 : 0);
}

static bool _GFXClipLine(struct GraphicsMode *gMode,int *x,int *y,int *x2,int *y2,bool *endClipped) {
	int xMax = gMode->xGSize - 1,yMax = gMode->yGSize - 1;
	int c1 = _GFXOutCode(gMode,*x,*y),c2 = _GFXOutCode(gMode,*x2,*y2);
	*endClipped = (c2 != 0);
	while (c1 | c2) {
		if (c1 & c2) return false;  													// Wholly off one side
		int c = c1 ? c1 : c2,nx = 0,ny = 0;
		long dx = *x2 - *x,dy = *y2 - *y;
		if (c & 8) { ny = yMax;nx = *x + (int)(dx * (yMax - *y) / dy); }
		else if (c & 4) { ny = 0;nx = *x + (int)(dx * (0 - *y) / dy); }
		else if (c & 2) { nx = xMax;ny = *y + (int)(dy * (xMax - *x) / dx); }
		else { nx = 0;ny = *y + (int)(dy * (0 - *x) / dx); }
		if (c == c1) { *x = nx;*y = ny;c1 = _GFXOutCode(gMode,nx,ny); }
		else { *x2 = nx;*y2 = ny;c2 = _GFXOutCode(gMode,nx,ny); }
	}
	return true;
}

void GFXFastLine(struct GraphicsMode *gMode,int x, int y, int x2, int y2) {

	_author++;
	
	bool endClipped;
	if (!_GFXClipLine(gMode,&x,&y,&x2,&y2,&endClipped)) return;  					// T-132 : clipped, not dropped
	if (endClipped) GFXPlotPixel(gMode,x2,y2);  									// The loop below stops BEFORE (x2,y2), as it always
																					// has ; once clipped, that end is the visible edge
	//		T-132 (0.16.92) : horizontal and vertical lines (rows of filled ellipses...) take the byte at a time
	//		paths of the rectangles : the same pixels as the loop below, (x2,y2) excluded.
	if (y == y2) {
		if (x2 > x) GFXHLine(gMode,x,x2 - 1,y); else if (x2 < x) GFXHLine(gMode,x2 + 1,x,y);
		return;
	}
	if (x == x2) {
		if (y2 > y) GFXVLine(gMode,x,y,y2 - 1); else GFXVLine(gMode,x,y2 + 1,y);
		return;
	}

	bool yLonger=false;
	int incrementVal, endVal;

	int shortLen=y2-y;
	int longLen=x2-x;
	if (abs(shortLen)>abs(longLen)) {
		int swap=shortLen;
		shortLen=longLen;
		longLen=swap;
		yLonger=true;
	}
	
	endVal=longLen;
	if (longLen<0) {
		incrementVal=-1;
		longLen=-longLen;
	} else incrementVal=1;

	double decInc;
	if (longLen==0) decInc=(double)shortLen;
	else decInc=((double)shortLen/(double)longLen);
	double j=0.0;
	if (yLonger) {
		for (int i=0;i!=endVal;i+=incrementVal) {
			GFXPlotPixel(gMode,x+(int)j,y+i);
			j+=decInc;
		}
	} else {
		for (int i=0;i!=endVal;i+=incrementVal) {
			GFXPlotPixel(gMode,x+i,y+(int)j);
			j+=decInc;
		}
	}
}

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//
// ***************************************************************************************
