# Top-down tree of shapes extraction #

## Summary ##
A top-down algorithm for the extraction of shapes. This is an alternative to the
classical FLST algorithm.

## Author ##

* Pascal Monasse <monasse@imagine.enpc.fr>

Laboratoire d'Informatique Gaspard Monge (LIGM)/
Ecole des Ponts ParisTech

## Version ##
Version 1.0, released on 06/18/2018

## License ##

This program is free software: you can redistribute it and/or modify
it under the terms of the Mozilla Public License as
published by the Mozilla Foundation, either version 2.0 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
Mozilla Public License for more details.

You should have received a copy of the Mozilla Public License
along with this program. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>

Copyright (C) 2018 Pascal Monasse <monasse@imagine.enpc.fr>

All rights reserved.

## Build ##
Required environment: Any unix-like system with a standard compilation
environment (make and C compiler) and [Cmake](https://cmake.org/).
Libraries:
[libpng](http://libpng.org/pub/png/libpng.html),
[lipjpeg](http://ijg.org/),
[libtiff](http://simplesystems.org/libtiff/),
[Imagine++](http://imagine.enpc.fr/~monasse/Imagine++/) (optional)

Build instructions:

    $ mkdir build
    $ cd build
    $ cmake -DCMAKE_BUILD_TYPE=Release ../src
    $ make

It produces library *Shape* and programs *main* (Imagine++ available) and *test_FLST*.

An important part of the used memory is due to the storage of contours (level lines). For large images, it may be better not to store them, which can be achieved by disabling the *Boundary* option:

    $ cmake -DBoundary=OFF .
    $ make

The same effect as the first line can be achieved by modifying the file CMakeCache.txt, either directly or through interactive tools ccmake and cmake-gui.

## Usage ##
Check everything is fine on toy dataset contained in folder data/:

    $ ./check_FLST

Launch with an image file as argument.

      Usage: ./test_FLST image

### Generating HTML documentation ###
    $ cd src
    $ doxygen Doxyfile

Open html/index.html to browse the documentation.

## List of source files  (folder src/) ##

* flst.cpp         : Main algorithm (library)
* shape.{h,cpp}    : Shape structure (library)
* tree.{h,cpp}     : Tree of shapes (library)
* check_FLST.cpp   : Sanity check program
* test_FLST.cpp    : Test program showing usage
* main.cpp         : Graphical exploration of the tree

Additional files:

* libImage/        : C++ library for opening images in any format
* test_oldFlst.cpp : Test program for old, classical FLST
* oldFlst.{h,cpp}  : Traditional FLST (just for comparison)
