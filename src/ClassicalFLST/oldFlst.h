/**
 * SPDX-License-Identifier: MPL-2.0+
 * @file oldFlst.h
 * @brief Tree extraction by the classical FLST.
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

#ifndef OLDFLST_H
#define OLDFLST_H

#include "../tree.h"

typedef struct cimage {
    int nrow, ncol;
    unsigned char* gray;
} *Cimage;

LsTree* ls_new_tree();
int fllt(int* pMinArea, int* pMaxArea, Cimage pCharImageInput, LsTree* pTree);

#endif
