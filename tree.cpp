#include "tree.h"
#include <cassert>

/// Constructor.
LsTree::LsTree(const unsigned char* gray, int w, int h) {
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

    flst_td(gray);
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
