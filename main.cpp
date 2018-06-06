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
    openWindow(im.width(), im.height());
    display(im);

    LsTree tree(im.data(), im.width(), im.height());

    int x, y;
    while(getMouse(x,y)==1) {
        LsShape* s = tree.smallest_shape(x,y);
        std::cout << s->area << std::endl;
        std::vector<LsPoint>::iterator it, end=s->contour.end();
        noRefreshBegin();
        display(im);
        for(it=s->contour.begin(); it!=end; ++it)
            drawPoint(it->x, it->y, RED);
        noRefreshEnd();
    }

    endGraphics();
    return 0;
}

