#include "box.h"
#include "helper.h"

uint8_t numBoxes;
Box boxes[MAX_BOXES];


static uint8_t box_check_xy_in_bounds(Box *box,
    uint8_t x, uint8_t y, uint8_t *destX, uint8_t *destY)
{
	uint8_t xmin, xmax;

	// We are supposed to determine the point (destX,destY) contained in
	// the given box which is closest to the point (x,y), and then return
	// some kind of "distance" between the two points.

	// First, we determine destY and a range (xmin to xmax) in which destX
	// is contained.
	if (y < box->uy) {
		// Point is above the box
		*destY = box->uy;
		xmin = box->ulx;
		xmax = box->urx;
	} else if (y >= box->ly) {
		// Point is below the box
		*destY = box->ly;
		xmin = box->llx;
		xmax = box->lrx;
	} else if (x >= box->ulx && x >= box->llx && x < box->urx && x < box->lrx) {
		// Point is strictly inside the box
		*destX = x;
		*destY = y;
		xmin = xmax = x;
	} else {
		// Point is to the left or right of the box,
		// so the y coordinate remains unchanged
		*destY = y;
		uint8_t ul = box->ulx;
		uint8_t ll = box->llx;
		uint8_t ur = box->urx;
		uint8_t lr = box->lrx;
		uint8_t top = box->uy;
		uint8_t bottom = box->ly;
		uint8_t cury;

		// Perform a binary search to determine the x coordinate.
		// Note: It would be possible to compute this value in a
		// single step simply by calculating the slope of the left
		// resp. right side and using that to find the correct
		// result. However, the original engine did use the search
		// approach, so we do that, too.
		do {
			xmin = (ul + ll) / 2;
			xmax = (ur + lr) / 2;
			cury = (top + bottom) / 2;

			if (cury < y) {
				top = cury;
				ul = xmin;
				ur = xmax;
			} else if (cury > y) {
				bottom = cury;
				ll = xmin;
				lr = xmax;
			}
		} while (cury != y);
	}

	// Now that we have limited the value of destX to a fixed
	// interval, it's a trivial matter to finally determine it.
	if (x < xmin) {
		*destX = xmin;
	} else if (x > xmax) {
		*destX = xmax;
	} else {
		*destX = x;
	}

	// Compute the distance of the points. We measure the
	// distance with a granularity of 8x8 blocks only (hence
	// yDist must be divided by 4, as we are using 8x2 pixels
	// blocks for actor coordinates).
	uint8_t xDist = ABS(x - *destX);
	uint8_t yDist = ABS(y - *destY) / 4;
	uint8_t dist;

	if (xDist < yDist)
		dist = (xDist >> 1) + yDist;
	else
		dist = (yDist >> 1) + xDist;

	return dist;
}


uint8_t boxes_adjust_xy(uint8_t *px, uint8_t *py)
{
    int8_t i = numBoxes - 1;
    uint8_t bestDist = 0xFF;
    uint8_t box = 0xFF;
    uint8_t x = *px, y = *py;
    for ( ; i >= 0 ; --i)
    {
		// if ((flags & kBoxInvisible) && !((flags & kBoxPlayerOnly) && !isPlayer()))
		// 	continue;
        uint8_t foundX, foundY;
        uint8_t dist = box_check_xy_in_bounds(&boxes[i], *px, *py, &foundX, &foundY);
		if (dist == 0) {
			*px = foundX;
			*py = foundY;
			box = i;
			break;
		}
		if (dist < bestDist) {
			bestDist = dist;
			*px = foundX;
			*py = foundY;
			box = i;
		}
    }

    return box;
}
