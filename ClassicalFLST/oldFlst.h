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
