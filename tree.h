#ifndef TREE_H
#define TREE_H

#include "shape.h"

/// Tree of shapes.
struct LsTree {
    LsTree(const unsigned char* gray, int w, int h);
    ~LsTree();

    unsigned char* build_image() const;
    LsShape* smallest_shape(int x, int y);

    int ncol, nrow; ///< Dimensions of image
    LsShape* shapes; ///< The array of shapes
    int iNbShapes; ///< The number of shapes

    /// For each pixel, the smallest shape containing it
    LsShape** smallestShape;
private:
    void flst_td(const unsigned char* gray); ///< Top-down algo
};

#endif
