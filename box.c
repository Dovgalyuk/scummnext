#include "box.h"
#include "helper.h"
#include "debug.h"

uint8_t numBoxes;
Box boxes[MAX_BOXES];
uint8_t boxesMatrix[MAX_BOXES * (MAX_BOXES + 1)];

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

/**
 * Return the square of the distance between two points.
 */
uint16_t sqrDist(int8_t x1, int8_t y1, int8_t x2, int8_t y2)
{
    uint16_t diffx = ABS(x1 - x2);
    // if (diffx >= 0x1000)
    //     return 0xFFFFFF;

    uint16_t diffy = ABS(y1 - y2);
    // if (diffy >= 0x1000)
    //     return 0xFFFFFF;

    return diffx * diffx + diffy * diffy;
}


/**
 * Find the point on a line segment which is closest to a given point.
 */
static void closestPtOnLine(uint8_t sx, uint8_t sy, uint8_t ex, uint8_t ey,
                    uint8_t x, uint8_t y, uint8_t *ox, uint8_t *oy)
{
	const int lxdiff = ex - sx;
	const int lydiff = ey - sy;

	if (ex == sx) {	// Vertical line?
		*ox = sx;
		*oy = y;
	} else if (ey == sy) {	// Horizontal line?
		*ox = x;
		*oy = sy;
	} else {
        DEBUG_ASSERT(0);
		// const int dist = lxdiff * lxdiff + lydiff * lydiff;
		// int a, b, c;
		// if (ABS(lxdiff) > ABS(lydiff)) {
		// 	a = sx * lydiff / lxdiff;
		// 	b = x * lxdiff / lydiff;

		// 	c = (a + b - sy + y) * lydiff * lxdiff / dist;

		// 	*ox = c;
		// 	*oy = c * lydiff / lxdiff - a + sy;
		// } else {
		// 	a = sy * lxdiff / lydiff;
		// 	b = y * lydiff / lxdiff;

		// 	c = (a + b - sx + x) * lydiff * lxdiff / dist;

		// 	*ox = c * lxdiff / lydiff - a + sx;
		// 	*oy = c;
		// }
	}

    if (ABS(lydiff) < ABS(lxdiff))
    {
        if (lxdiff > 0) {
            if (*ox < sx)
            {
                *ox = sx;
                *oy = sy;
            }
            else if (*ox > ex)
            {
                *ox = ex;
                *oy = ey;
            }
        } else {
            if (*ox > sx)
            {
                *ox = sx;
                *oy = sy;
            }
            else if (*ox < ex)
            {
                *ox = ex;
                *oy = ey;
            }
        }
    } else {
        if (lydiff > 0) {
            if (*oy < sy)
            {
                *ox = sx;
                *oy = sy;
            }
            else if (*oy > ey)
            {
                *ox = ex;
                *oy = ey;
            }
        } else {
            if (*oy > sy)
            {
                *ox = sx;
                *oy = sy;
            }
            else if (*oy < ey)
            {
                *ox = ex;
                *oy = ey;
            }
        }
    }
}

uint8_t box_getClosestPtOnBox(uint8_t b, uint8_t x, uint8_t y, uint8_t *outX, uint8_t *outY)
{
    Box *box = &boxes[b];
    uint8_t tx, ty;
    uint16_t dist;
    uint16_t bestdist = 0xFFFF;

    closestPtOnLine(box->ulx, box->uy, box->urx, box->uy, x, y, &tx, &ty);
    dist = sqrDist(x, y, tx, ty);
    if (dist < bestdist) {
        bestdist = dist;
        *outX = tx;
        *outY = ty;
    }

    closestPtOnLine(box->urx, box->uy, box->lrx, box->ly, x, y, &tx, &ty);
    dist = sqrDist(x, y, tx, ty);
    if (dist < bestdist) {
        bestdist = dist;
        *outX = tx;
        *outY = ty;
    }

    closestPtOnLine(box->lrx, box->ly, box->llx, box->ly, x, y, &tx, &ty);
    dist = sqrDist(x, y, tx, ty);
    if (dist < bestdist) {
        bestdist = dist;
        *outX = tx;
        *outY = ty;
    }

    closestPtOnLine(box->llx, box->ly, box->ulx, box->uy, x, y, &tx, &ty);
    dist = sqrDist(x, y, tx, ty);
    if (dist < bestdist) {
        bestdist = dist;
        *outX = tx;
        *outY = ty;
    }

    return bestdist;
}

uint8_t box_checkXYInBounds(uint8_t b, uint8_t x, uint8_t y)
{
    DEBUG_ASSERT(b < MAX_BOXES);
    Box *box = &boxes[b];

    // TODO: support non-rectangular
    DEBUG_ASSERT(box->ulx == box->llx);
    DEBUG_ASSERT(box->urx == box->lrx);

	// Quick check: If the x (resp. y) coordinate of the point is
	// strictly smaller (bigger) than the x (y) coordinates of all
	// corners of the quadrangle, then it certainly is *not* contained
	// inside the quadrangle.
	if (x < box->ulx && x < box->urx && x < box->lrx && x < box->llx)
		return 0;

	if (x > box->ulx && x > box->urx && x > box->lrx && x > box->llx)
		return 0;

	if (y < box->uy && y < box->ly)
		return 0;

	if (y > box->uy && y > box->ly)
		return 0;

    return 1;
}


uint8_t boxes_adjustXY(uint8_t *px, uint8_t *py)
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
        uint8_t dist = box_check_xy_in_bounds(&boxes[i], x, y, &foundX, &foundY);
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

/**
 * Compute if there is a way that connects box 'from' with box 'to'.
 * Returns the number of a box adjacent to 'from' that is the next on the
 * way to 'to' (this can be 'to' itself or a third box).
 * If there is no connection -1 is return.
 */
int8_t boxes_getNext(int8_t from, int8_t to)
{
    uint8_t *boxm = boxesMatrix;

	if (from == to)
		return to;

	if (to == -1)
		return -1;

	if (from == -1)
		return to;

    // The v2 box matrix is a real matrix with numOfBoxes rows and columns.
    // The first numOfBoxes bytes contain indices to the start of the corresponding
    // row (although that seems unnecessary to me - the value is easily computable.
    boxm += numBoxes + boxm[from];
    return boxm[to];
}
