/**
 * SPDX-License-Identifier: MPL-2.0+
 * @file edgel.cpp
 * @brief Boundary between 4-adjacent pixels
 * @author Pascal Monasse <monasse@imagine.enpc.fr>
 *
 * Copyright (c) 2018 Pascal Monasse
 * All rights reserved.
 */

#include "edgel.h"

/// Make a 180 turn compared to direction \a dir.
static DirEdgel turn_180(DirEdgel dir) {
    if(dir >= DIAGONAL) {
        dir -= 2;
        if(dir < DIAGONAL)
            dir += DIAGONAL;
    } else if(dir < 2)
        dir += 2;
    else
        dir -= 2;
    return dir;
}

/// Constructor.
Edgel::Edgel(short int x, short int y, DirEdgel d)
: pt(), dir(d) {
    pt.x = x;
    pt.y = y;
}

/// Change to inverse edgel
bool Edgel::inverse(Cimage im) {
    if(! exterior(pt, im))
        return false;
    dir = turn_180(dir);
    return true;
}

/// Exterior pixel of edgel.
/// Return \a false if we are an image boundary edgel.
bool Edgel::exterior(LsPoint& ext, Cimage im) const {
    ext = pt;
    switch(dir) {
    case EAST:  return (++ext.y < im->nrow);
    case NORTH: return (++ext.x < im->ncol);
    case WEST:  return (--ext.y >= 0);
    case SOUTH: return (--ext.x >= 0);
    case NE: return (++ext.y < im->nrow && ++ext.x < im->ncol);
    case NW: return (++ext.x < im->ncol && --ext.y >= 0);
    case SW: return (--ext.y >= 0 && --ext.x >= 0);
    case SE: return (--ext.x >= 0 && ++ext.y < im->nrow);
    default: assert(false);
    }
    return false;
}

/// Go straight along current direction.
/// Return \c false if we end up outside the image.
bool Edgel::go_straight(Cimage im) {
    switch(dir) {
    case EAST:  return (++pt.x < im->ncol);
    case NORTH: return (--pt.y >= 0);
    case WEST:  return (--pt.x >= 0);
    case SOUTH: return (++pt.y < im->nrow);
    default: assert(false);
    }
    return false;
}

/// Begin a left turn.
void Edgel::turn_left(int connect) {
    if(connect == 8)
        dir += DIAGONAL;
    else if(++dir == DIAGONAL)
        dir = 0;
}

/// Begin a right turn.
void Edgel::turn_right(int connect) {
    if(connect == 8) {
        if(dir == 0)
            dir = DIAGONAL;
        --dir;
    } else {
        dir += DIAGONAL-1;
        if(dir < DIAGONAL)
            dir += DIAGONAL;
    }
}

/// Move to next edgel along the level line.
void Edgel::next(Cimage im, LsShape::Type type, int level) {
    int connect = connectivity(type);
    if(dir >= DIAGONAL) {
        finish_turn(im, connect);
        return;
    }
    Edgel left(*this), right(*this);
    bool bLeftIn = left.go_straight(im), bRightIn = false;
    if(bLeftIn) {
        unsigned char v = gray(im, left.pt);
        bLeftIn = COMPARE(type, v, level);
        bRightIn = left.exterior(right.pt, im);
        if(bRightIn) {
            v = gray(im,right.pt);
            bRightIn = COMPARE(type, v, level);
        }
    }
    if(bLeftIn && ! bRightIn) // Go straight
        *this = left;
    else if(! bLeftIn && (! bRightIn || connect == 4))
        turn_left(connect);
    else {
        *this = (connect==4)? left: right;
        turn_right(connect);
    }
}
