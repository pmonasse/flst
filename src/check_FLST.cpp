/**
 * SPDX-License-Identifier: MPL-2.0+
 * @file check_FLST.cpp
 * @brief Check FLST on simple dataset.
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
#include <algorithm>
#include <cstdlib>
#include <iostream>

#define FOLDER "../data/"

int main() {
    const char* name;
    Image<unsigned char> im;

    if(! libs::ReadImage(name=FOLDER "check1.png", &im)) {
        std::cerr << "Error loading image " << name << std::endl;
        return 1;
    }
    {
        LsTree tree(im.data(), im.Width(), im.Height());
        std::cout << "* " << name << std::endl;
        std::cout << "Shapes (=2): " << tree.iNbShapes << std::endl;
        std::cout << "Pixels leaf (=3000): " << tree.shapes[1].area << std::endl;
    }

    if(! libs::ReadImage(name=FOLDER "check2.png", &im)) {
        std::cerr << "Error loading image " << name << std::endl;
        return 1;
    }
    {
        LsTree tree(im.data(), im.Width(), im.Height());
        std::cout << "* " << name << std::endl;
        std::cout << "Shapes (=3): " << tree.iNbShapes << std::endl;
        std::cout << "Pixels intermediate shape (=3000): " << tree.shapes[0].child->area << std::endl;
        std::cout << "Pixels leaf (=900): " << tree.shapes[0].child->child->area << std::endl;
    }

    if(! libs::ReadImage(name=FOLDER "check3.png", &im)) {
        std::cerr << "Error loading image " << name << std::endl;
        return 1;
    }
    {
        LsTree tree(im.data(), im.Width(), im.Height());
        std::cout << "* " << name << std::endl;
        std::cout << "Shapes (=4): " << tree.iNbShapes << std::endl;
        std::cout << "Pixels intermediate shape (=3000): " << tree.shapes[0].child->area << std::endl;
        LsShape *s1=tree.shapes[0].child->child, *s2=s1->sibling;
        if(s1->type == LsShape::INF)
            std::swap(s1,s2);
        std::cout << "Pixels leaf1 (=900): " << s1->area << std::endl;
        std::cout << "Pixels leaf2 (=500): " << s2->area << std::endl;        
    }

    if(! libs::ReadImage(name=FOLDER "check4.png", &im)) {
        std::cerr << "Error loading image " << name << std::endl;
        return 1;
    }
    {
        LsTree tree(im.data(), im.Width(), im.Height());
        std::cout << "* " << name << std::endl;
        std::cout << "Shapes (=4): " << tree.iNbShapes << std::endl;
        std::cout << "Pixels intermediate shape (=3000): " << tree.shapes[0].child->area << std::endl;
        LsShape *s1=tree.shapes[0].child->child, *s2=s1->sibling;
        if(s1->type == LsShape::INF)
            std::swap(s1,s2);
        std::cout << "Pixels leaf1 (=700): " << s1->area << std::endl;
        std::cout << "Pixels leaf2 (=500): " << s2->area << std::endl;        
    }

    if(! libs::ReadImage(name=FOLDER "check5.png", &im)) {
        std::cerr << "Error loading image " << name << std::endl;
        return 1;
    }
    {
        LsTree tree(im.data(), im.Width(), im.Height());
        std::cout << "* " << name << std::endl;
        std::cout << "Shapes (=4): " << tree.iNbShapes << std::endl;
        std::cout << "Pixels intermediate shape (=3000): " << tree.shapes[0].child->area << std::endl;
        LsShape *s1=tree.shapes[0].child->child;
        std::cout << "Pixels double-L (=1000): " << s1->area << std::endl;
        s1 = s1->child;
        std::cout << "Pixels leaf (=200): " << s1->area << std::endl;
    }

    if(! libs::ReadImage(name=FOLDER "check6.png", &im)) {
        std::cerr << "Error loading image " << name << std::endl;
        return 1;
    }
    {
        LsTree tree(im.data(), im.Width(), im.Height());
        std::cout << "* " << name << std::endl;
        std::cout << "Shapes (=4): " << tree.iNbShapes << std::endl;
        std::cout << "Pixels intermediate shape (=3000): " << tree.shapes[0].child->area << std::endl;
        LsShape *s1=tree.shapes[0].child->child, *s2=s1->sibling;
        std::cout << "Pixels leaf1 (=400): " << s1->area << std::endl;
        std::cout << "Pixels leaf2 (=400): " << s2->area << std::endl;
    }

    if(! libs::ReadImage(name=FOLDER "check7.png", &im)) {
        std::cerr << "Error loading image " << name << std::endl;
        return 1;
    }
    {
        LsTree tree(im.data(), im.Width(), im.Height());
        std::cout << "* " << name << std::endl;
        std::cout << "Shapes (=4): " << tree.iNbShapes << std::endl;
        std::cout << "Pixels child (=2500): " << tree.shapes[0].child->area << std::endl;
        LsShape *s1=tree.shapes[0].child->child, *s2=s1->child;
        std::cout << "Pixels grand-child (=2400): " << s1->area << std::endl;
        std::cout << "Pixels leaf (=2300): " << s2->area << std::endl;
    }

    if(! libs::ReadImage(name=FOLDER "check8.png", &im)) {
        std::cerr << "Error loading image " << name << std::endl;
        return 1;
    }
    {
        LsTree tree(im.data(), im.Width(), im.Height());
        std::cout << "* " << name << std::endl;
        std::cout << "Shapes (=4): " << tree.iNbShapes << std::endl;
        std::cout << "Pixels child (=2500): " << tree.shapes[0].child->area << std::endl;
        LsShape *s1=tree.shapes[0].child->child, *s2=s1->child;
        std::cout << "Pixels grand-child (=2400): " << s1->area << std::endl;
        std::cout << "Pixels leaf (=100): " << s2->area << std::endl;
    }
    return 0;
}
