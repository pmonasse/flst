/**
 * SPDX-License-Identifier: MPL-2.0+
 * @file edgel.h
 * @brief Boundary between 4-adjacent pixels
 * @author Pascal Monasse <monasse@imagine.enpc.fr>
 *
 * Copyright (c) 2018 Pascal Monasse
 * All rights reserved.
 */

#ifndef EDGEL_H
#define EDGEL_H

#include "shape.h"
#include <cassert>

struct cimage {
    int nrow, ncol;
    const unsigned char* gray;
};
typedef cimage* Cimage;
inline unsigned char gray(Cimage im, LsPoint pt)
{ return im->gray[pt.y*im->ncol+pt.x]; }

/// Strict comparison between numbers
#define COMPARE(t,a,b) (t==LsShape::INF? (a<b): (a>b))

/// Return connectivity for level set of type \a t.
inline int connectivity(LsShape::Type t) {
    return ((t == LsShape::INF)? 4: 8);
}

/// Direction of an edgel.
typedef unsigned char DirEdgel;
static const DirEdgel EAST  = 0;
static const DirEdgel NORTH = 1;
static const DirEdgel WEST  = 2;
static const DirEdgel SOUTH = 3;
static const DirEdgel DIAGONAL = 4;
static const DirEdgel NE = 4;
static const DirEdgel NW = 5;
static const DirEdgel SW = 6;
static const DirEdgel SE = 7;

/// Edgel, vertical or horizontal boundary between adjacent pixels.
class Edgel {
public:
    Edgel(short int x, short int y, DirEdgel d);

    bool operator==(const Edgel& e) const
    { return (pt.x == e.pt.x && pt.y == e.pt.y && dir == e.dir); }
    bool operator!=(const Edgel& e) const
    { return ! (e == *this); }

    bool inverse(Cimage im);
    LsPoint origin() const;
    bool exterior(LsPoint& ext, Cimage im) const;
    bool go_straight(Cimage im);
    void next(Cimage im, LsShape::Type type, int level);

    LsPoint pt; ///< Interior pixel coordinates (left of edgel direction)
    DirEdgel dir; ///< Direction of edgel
private:
    void turn_left(int connect);
    void turn_right(int connect);
    void finish_turn(Cimage im, int connect);
};

/// Finish a left or right turn.
inline void Edgel::finish_turn(Cimage im, int connect) {
    dir -= DIAGONAL;
    if(connect == 4)
        go_straight(im);
    else if(++dir == DIAGONAL)
        dir = 0;
}

/// Return coordinates of origin of edgel.
inline LsPoint Edgel::origin() const {
    assert(dir<DIAGONAL);
    LsPoint p = pt;
    if(dir == EAST || dir == NORTH)
        ++p.y;
    if(dir == NORTH || dir == WEST)
        ++p.x;
    return p;
}

#endif
