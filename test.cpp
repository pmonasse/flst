/**
 * SPDX-License-Identifier: MPL-2.0+
 * @file test.cpp
 * @brief Basic usage example for tree extraction.
 * @author Pascal Monasse <monasse@imagine.enpc.fr>
 *
 * Copyright (c) 2018 Pascal Monasse
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the Mozilla Public License as
 * published by the Mozilla Foundation, either version 2.0 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Mozilla Public License for more details.
 *
 * You should have received a copy of the Mozilla Public License
 * along with this program. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>
 */

#include "libImage/image_io.hpp"
#include "tree.h"
#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[]) {
    if(argc!=2) {
        std::cerr << "Usage: " << argv[0] << " imageFile" << std::endl;
        return 1;
    }
    Image<unsigned char> im;
    if(! libs::ReadImage(argv[1], &im)) {
        std::cerr << "Error loading image " << argv[1] << std::endl;
        return 1;
    }

    LsTree tree(im.data(), im.Width(), im.Height());
    std::cout << "Shapes: " << tree.iNbShapes << " "
              << "Mem: " << (tree.iNbShapes*sizeof(LsShape)+tree.nrow*tree.ncol*sizeof(LsShape*))/1024/1024 <<  "MB ";

    long int TV=0;
    for(int i=0; i<im.Height(); i++)
        for(int j=0; j+1<im.Width(); j++)
            TV += abs(im(i,j)-im(i,j+1));
    for(int i=0; i+1<im.Height(); i++)
        for(int j=0; j<im.Width(); j++)
            TV += abs(im(i,j)-im(i+1,j));
    std::cout << "TV: " << TV << std::endl;

    return 0;
}
