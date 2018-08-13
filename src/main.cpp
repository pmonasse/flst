/**
 * SPDX-License-Identifier: MPL-2.0+
 * @file main.cpp
 * @brief Tree extraction and visual exploration, requiring Imagine library.
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

#include <Imagine/Images.h>
#include "tree.h"
#include <iostream>
using namespace Imagine;

int main(int argc, char* argv[]) {
    if(argc!=2) {
        std::cerr << "Usage: " << argv[0] << " imageFile" << std::endl;
        return 1;
    }
    Image<byte> im;
    if(! load(im, argv[1])) {
        std::cerr << "Error loading image " << argv[1] << std::endl;
        return 1;
    }
    openWindow(im.width()+1, im.height()+1);
    display(im);

    LsTree tree(im.data(), im.width(), im.height());
    std::cout << "Shapes: " << tree.iNbShapes << std::endl;

    int x, y;
    while(getMouse(x,y)==1) {
        if(x>=im.width() || y>= im.height()) continue;
        LsShape* s = tree.smallest_shape(x,y);
        std::cout << s->area << std::endl;
        noRefreshBegin();
        clearWindow();
        display(im);
#if BOUNDARY
        Color col = (s->type==LsShape::INF) ? BLUE: RED;
        std::vector<LsPoint>::iterator it, end=s->contour.end();
        for(it=s->contour.begin(); it!=end; ++it)
            drawPoint(it->x, it->y, col);
#endif
        noRefreshEnd();
    }

    endGraphics();
    return 0;
}
