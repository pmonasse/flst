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
