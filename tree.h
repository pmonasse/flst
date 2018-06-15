/**
 * SPDX-License-Identifier: MPL-2.0
 * @file tree.h
 * @brief Tree of shapes
 * @author Pascal Monasse <monasse@imagine.enpc.fr>
 *
 * Copyright (c) 2018 Pascal Monasse
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * You should have received a copy of the GNU General Pulic License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TREE_H
#define TREE_H

#include "shape.h"

/// Tree of shapes.
struct LsTree {
    LsTree() {} //For use with old FLST only
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
