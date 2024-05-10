/**
 * SPDX-License-Identifier: MPL-2.0+
 * @file flst_song.cpp
 * @brief Yuqing Song's top-down tree extraction
 * @author Pascal Monasse <monasse@imagine.enpc.fr>
 *
 * Copyright (c) 2024 Pascal Monasse
 * All rights reserved.
 */

#include "edgel.h"
#include "tree.h"
#include <stack>

/// Fix initial edgel to be one of 4 cardinal directions.
/// level must be strictly between the gray levels of e.pt and e's exterior.
/// The diagonal points have gray \a level or same side as e.pt.
static void fix_initial_edgel(Cimage im, LsShape::Type t, Edgel& e, int level) {
    assert(e.dir>=DIAGONAL);
    LsPoint ext;
    bool bExteriorExist = e.exterior(ext, im);
    assert(bExteriorExist);
    LsPoint diag1 = {e.pt.x,ext.y}, diag2 = {ext.x,e.pt.y};
    if(COMPARE(t,gray(im,diag1),level)) {
        e.pt = diag1;
        e.dir = (diag1.x<ext.x)? NORTH: SOUTH;
    } else if(COMPARE(t,gray(im,diag2),level)) {
        e.pt = diag2;
        e.dir = (diag2.y<ext.y)? EAST: WEST;
    } else {
        e.dir = (e.pt.x<ext.x)? NORTH: SOUTH;
    }
}

/// Find largest shape \a s with boundary containing \a e. Return this boundary
/// as a sequence of edgels. \a level is the gray level of the parent.
/// Fields \c pixels, \c parent, \c sibling and \c child are not set.
static std::vector<Edgel> locate_line(Cimage im, LsShape& s,
                                      Edgel e, int level) {
    s.type = (gray(im,e.pt) < level)? LsShape::INF: LsShape::SUP;
    s.gray = (s.type==LsShape::INF)? 0: 255;
    s.bIgnore = false;
    s.bBoundary = false;

    if(e.dir>=DIAGONAL) // Avoid infinite loop
        fix_initial_edgel(im, s.type, e, level);
        //        e.next(im, s.type, level);

    std::vector<Edgel> boundary;
    Edgel cur = e;
    do {
        boundary.push_back(cur);
#ifdef BOUNDARY
        if(cur.dir < DIAGONAL)
            s.contour.push_back(cur.origin());
#endif
        unsigned char v = gray(im, cur.pt);
        if(! COMPARE(s.type, v, s.gray))
            s.gray = v;
        cur.next(im, s.type, level);
    } while(cur != e);

    return boundary;
}

/// Add exterior pixel q of edgel \a e to \a Qp if its gray level is \a g,
/// otherwise add inverse of \a e in Qc. Nothing happens if q has already been
/// discovered (\a color is 0).
static void classify_exterior(Cimage im, Cimage color,
                              Edgel e, unsigned char g,
                              std::stack<LsPoint>& Qp,
                              std::stack<Edgel>& Qc) {
    Edgel f(e);
    if(!f.inverse(im) || gray(color,f.pt)!=0)
        return;
    if(gray(im,f.pt)==g)
        Qp.push(f.pt);
    else
        Qc.push(f);
    color->gray[f.pt.y*color->ncol+f.pt.x] = 1;
}

/// Fill subtree rooted at the last shape of \a tree, with boundary \a bound.
/// Parameter \a color is a flag marking explored pixels.
static void locate_all_children(Cimage im, LsTree& tree,
                                const std::vector<Edgel>& bound,
                                Cimage color) {
    LsShape& s = tree.shapes[tree.iNbShapes-1];
    s.area = 0;
    if(s.parent) {
        LsPoint* cEnd=s.parent->pixels;
        for(LsShape* c=s.child; c!=0; c=c->sibling)
            if(cEnd<c->pixels+c->area)
                cEnd = c->pixels+c->area;
        s.pixels = cEnd;
    }
    std::stack<LsPoint> Qp; // Private pixels
    std::stack<Edgel> Qc; // Edgels for children
    std::vector<LsPoint> pp; // Private region of s
    std::vector<Edgel>::const_iterator it=bound.begin(), end=bound.end();
    for(; it!=end; ++it) {
        if(tree.smallestShape[it->pt.y*tree.ncol+it->pt.x])
            continue;
        if(gray(im,it->pt)==s.gray)
            Qp.push(it->pt);
        else
            Qc.push(*it);
        color->gray[it->pt.y*color->ncol+it->pt.x] = 1;
        while(! (Qp.empty() && Qc.empty())) {
            if(! Qp.empty()) {
                Edgel e(Qp.top()); Qp.pop();
                int idx = e.pt.y*color->ncol+e.pt.x;
                color->gray[idx] = 2;
                tree.smallestShape[idx] = &s;
                pp.push_back(e.pt);
                for(e.dir=0; e.dir!=DIAGONAL; e.dir++) // Scan neighbors
                    classify_exterior(im, color, e, s.gray, Qp, Qc);
            }
            if(! Qc.empty()) {
                Edgel e(Qc.top()); Qc.pop();
                if(gray(color,e.pt)==2)
                    continue;
                LsShape* c = tree.add_child(s);
                std::vector<Edgel> b = locate_line(im, *c, e, s.gray);
                std::vector<Edgel>::const_iterator bc=b.begin(), be=b.end();
                for(; bc!=be; ++bc) {
                    color->gray[bc->pt.y*color->ncol+bc->pt.x] = 2;
                    classify_exterior(im, color, *bc, s.gray, Qp, Qc);
                }
                locate_all_children(im, tree, b, color);
                s.area += c->area;
            }
        }
    }
    std::copy(pp.begin(), pp.end(), s.pixels+s.area);
    s.area += (int)pp.size();
}

/// Top-down post-order FLST algorithm. Children are built immediately on
/// detection, private pixels are stored after.
void LsTree::flst_td_post(const unsigned char* gray) {
    cimage image = {nrow, ncol, (unsigned char*)gray};
    int area = ncol * nrow;

    std::fill(smallestShape, smallestShape+area, (LsShape*)0);

    cimage color = {nrow, ncol, new unsigned char[area]};
    std::fill(color.gray, color.gray+area, (unsigned char)0);

    shapes[0].type = LsShape::SUP;
    shapes[0].pixels = new LsPoint[area];
    Edgel e(0, 0, SOUTH);
    std::vector<Edgel> bound = locate_line(&image, shapes[0], e, -1);
    locate_all_children(&image, *this, bound, &color);
    assert(area == shapes[0].area);
    delete [] color.gray;
    fill_bBoundary();
}
