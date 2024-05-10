/**
 * SPDX-License-Identifier: MPL-2.0+
 * @file tree.h
 * @brief Tree of shapes
 * @author Pascal Monasse <monasse@imagine.enpc.fr>
 *
 * Copyright (c) 2018,2024 Pascal Monasse
 * All rights reserved.
 */

#ifndef TREE_H
#define TREE_H

#include "shape.h"

/// Tree of shapes.
struct LsTree {
    typedef enum {TD_PRE, TD_POST} Algo;
    LsTree() {} //For use with old FLST only
    LsTree(const unsigned char* gray, int w, int h, Algo algo=TD_PRE);
    ~LsTree();

    unsigned char* build_image() const;
    LsShape* smallest_shape(int x, int y);
    LsShape* add_child(LsShape& parent);

    int ncol, nrow; ///< Dimensions of image
    LsShape* shapes; ///< The array of shapes
    int iNbShapes; ///< The number of shapes

    /// For each pixel, the smallest shape containing it
    LsShape** smallestShape;
private:
    void index_smallestShape();
    void fill_bBoundary();
    void flst_td_pre(const unsigned char* gray); ///< Top-down pre-order algo
    void flst_td_post(const unsigned char* gray); ///< Top-down post-order algo
};

#endif
