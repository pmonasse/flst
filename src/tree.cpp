/**
 * SPDX-License-Identifier: MPL-2.0+
 * @file tree.cpp
 * @brief Tree of shapes
 * @author Pascal Monasse <monasse@imagine.enpc.fr>
 *
 * Copyright (c) 2018,2024 Pascal Monasse
 * All rights reserved.
 */

#include "tree.h"
#include <cassert>

/// \brief Regular constructor.
/// \details The tree is built from here, calling the method \a flst_td.
LsTree::LsTree(const unsigned char* gray, int w, int h, LsTree::Algo algo) {
    nrow = h; ncol = w;

    // Set the root of the tree. #shapes <= #pixels
    LsShape* pRoot = shapes = new LsShape[nrow*ncol];
    pRoot->type = LsShape::INF; pRoot->gray = 255;
    pRoot->bBoundary = true;
    pRoot->bIgnore = false;
    pRoot->area = nrow*ncol;
    pRoot->parent = pRoot->sibling = pRoot->child = 0;
    pRoot->pixels = 0;
    iNbShapes = 1;

    smallestShape = new LsShape*[ncol*nrow];
    for(int i = ncol*nrow-1; i >= 0; i--)
        smallestShape[i] = pRoot;

    if(algo == TD_PRE)
        flst_td_pre(gray);
    else if(algo == TD_POST)
        flst_td_post(gray);
    else
        assert(false);
}

/// Destructor.
LsTree::~LsTree() {
    if(shapes && iNbShapes > 0)
        delete [] shapes[0].pixels;
    delete [] shapes;
    delete [] smallestShape;
}

/// Reconstruct an image from the tree
unsigned char* LsTree::build_image() const {
    unsigned char* gray = new unsigned char[nrow*ncol];
    unsigned char* out = gray;
    LsShape** ppShape = smallestShape;
    for(int i = nrow*ncol-1; i >= 0; i--) {
        LsShape* pShape = *ppShape++;
        while(pShape->bIgnore)
            pShape = pShape->parent;
        *out++ = pShape->gray;
    }
    return gray;
}

/// Smallest non-removed shape at pixel (\a x,\a y).
LsShape* LsTree::smallest_shape(int x, int y) {
    LsShape* pShape = smallestShape[y*ncol + x];
    if(pShape->bIgnore)
        pShape = pShape->find_parent();
    return pShape;
}

/// Add a new child to shape \a parent.
/// Fields other than family pointers are left uninitialized.
/// No allocation is performed, the new shape is placed at shapes[iNbShapes],
/// hence after the call the returned shape is simply shapes[iNbShapes-1].
LsShape* LsTree::add_child(LsShape& parent) {
    assert(iNbShapes < nrow*ncol);
    LsShape* old = parent.child;
    parent.child = &shapes[iNbShapes++];
    parent.child->parent = &parent;
    parent.child->sibling = old;
    parent.child->child = 0;
    return parent.child;
}

/// Index \a smallestShapeRecursive from tree rooted at \a s.
static void index(LsShape* s, LsShape** smallestShape, int w) {
    for(LsShape* c=s->child; c; c=c->sibling)
        index(c, smallestShape, w);
    // Fill private pixels, located either before or after all children's pixels
    LsPoint *cBegin=s->pixels+s->area, *cEnd=s->pixels;
    for(LsShape* c=s->child; c; c=c->sibling) {
        if(cBegin>c->pixels)
            cBegin = c->pixels;
        if(cEnd<c->pixels+c->area)
            cEnd = c->pixels+c->area;
    }
    for(LsPoint* p=s->pixels; p<cBegin; p++)
        smallestShape[p->y*w+p->x] = s;
    for(LsPoint* p=cEnd; p<s->pixels+s->area; p++)
        smallestShape[p->y*w+p->x] = s;
}

/// Fill the index tree.smallestShape (supposed to be already allocated) based
/// on the field \c pixels of each shape.
void LsTree::index_smallestShape() {
    assert(smallestShape!=0);
    index(shapes, smallestShape, ncol);
}

/// Tag shapes meeting image boundary (use \c smallestShape, field \c bBoundary)
void LsTree::fill_bBoundary() {
    LsTreeIterator it,end(LsTreeIterator::Post, shapes);
    for(it=LsTreeIterator(LsTreeIterator::Post, shapes); it!=end; ++it)
        (*it)->bBoundary = false;
    for(int x=0; x<ncol; x++)
        smallestShape[0*ncol+x]->bBoundary = true;
    for(int x=0; x<ncol; x++)
        smallestShape[(nrow-1)*ncol+x]->bBoundary = true;
    for(int y=1; y+1<nrow; y++) {
        smallestShape[y*ncol+0]->bBoundary = true; // First of row y
        smallestShape[(y+1)*ncol-1]->bBoundary = true; // Last of row y
    }
    // Propagate up-tree
    for(it=LsTreeIterator(LsTreeIterator::Post, shapes); it!=end; ++it)
        if((*it)->parent && (*it)->bBoundary)
            (*it)->parent->bBoundary = true;
}
