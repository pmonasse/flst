/**
 * SPDX-License-Identifier: MPL-2.0+
 * @file main.cpp
 * @brief Tree extraction and visual exploration, requiring Imagine library.
 * @author Pascal Monasse <monasse@imagine.enpc.fr>
 *
 * Copyright (c) 2018 Pascal Monasse
 * All rights reserved.
 */

#include <Imagine/Images.h>
#include "tree.h"
#include <iostream>
using namespace Imagine;

int main(int argc, char* argv[]) {
    if(argc!=2 && argc!=3) {
        std::cerr << "Usage: " << argv[0] << " imageFile [algo]" << std::endl;
        std::cerr << "Algo: one of PRE, POST. Default: PRE" << std::endl;
        return 1;
    }
    Image<byte> im;
    if(! load(im, argv[1])) {
        std::cerr << "Error loading image " << argv[1] << std::endl;
        return 1;
    }
    LsTree::Algo algo = LsTree::TD_PRE;
    if(argc>2) {
        if(argv[2]==std::string("POST"))
            algo = LsTree::TD_POST;
        else if(argv[2]!=std::string("PRE")) {
            std::cerr << "Unknown algo " << argv[2] << std::endl;
            return 1;
        }
    }
    
    openWindow(im.width()+1, im.height()+1);
    display(im);

    LsTree tree(im.data(), im.width(), im.height(), algo);
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
