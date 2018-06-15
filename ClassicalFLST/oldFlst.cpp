/**
 * SPDX-License-Identifier: MPL-2.0+
 * @file oldFlst.cpp
 * @brief Tree extraction by the classical FLST. Sorry, ugly code...
 * @author Pascal Monasse <monasse@imagine.enpc.fr>
 *
 * Copyright (c) 1998-2018 Pascal Monasse
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map> //STL
#include <assert.h>
#include "oldFlst.h"

#define mwerror(x,y,str) printf(str)
void ls_delete_tree(LsTree* pTree);


#define UP 1
#define LEFT 4
#define DOWN 16
#define RIGHT 64

/* These (diagonal) directions are not directions of frontiers (invalid values for the pixels of frontiers),
   but are used to discriminate the local configurations of the frontier (to count the number of cc of
   the frontier), indicating how are the diagonal pixels (in or out the shape). */
#define UPLEFT 2
#define LEFTDOWN 8
#define DOWNRIGHT 32
#define RIGHTUP 128

/* The structure to encode the configuration of the frontier of a shape. The member cDirections represents
   the directions of separation between this pixel and up and left neighbors. The orientations indicate
   in which side of the direction is the interior of the shape. The configurations: (pixels in: *, out: o)
   --------------------------------------------------------------------------------------------
   |      o             *             *                       o                               |
   |    * o -> DOWN   o o -> LEFT   * o  -> LEFT | DOWN     o o  -> 0 (not a frontier pixel)  |
   |                                                                                          |
   |      *             o             o                       *                               |
   |    o * -> UP     * * -> RIGHT  o *  -> UP | RIGHT      * *  -> 0 (not a frontier pixel)  |
   --------------------------------------------------------------------------------------------
    Notice that this way, we need to have an image of frontiers of 1 line and 1 column larger than
    the original image.
    We keep an index iExploration in memory, which is used to know when the pixel was initialised for the
    last time (so that we do not need to clear the image of frontiers at each new shape).
    In the same spirit, iExplorationHole is used to dicriminate the last time the pixel was explored (shapes
    starting from the same local extremum have the same index iExploration, so this iExplorationHole is
    used when we need to follow the frontier, for shapes with hole). cCurrentDirections is the current
    configuration of the frontier, that is cDirections minus the directions already explored (we erase the
    directions already explored). */
struct FlstFrontierPixel
{
  int iExploration; /* To know the last time this frontier pixel was initialised (for which local extremum)*/
  unsigned char cDirections; /* The coded local configuration of the frontier of the level set. */
};

/* To know if a neighbor pixel was already visited for this local extremum */
#define NOTVISITED(x,y) (visitefront[y][x] < iIndexOfExploration)

/* The structure representing the neighborhood of a region. It is made of all the neighbors and of a
   smart indexing structure to access easily the ones with a given gray-value. For each gray-level g, we
   know a pointer to a pixel having this gray-level (given by tabpFirstNeighbor[g]) and the other neighbors
   with the same gray-level are accessible following the list (pixels of same gray-level are linked). When
   pixels are removed from the neighbor list (because they are incorporated in the growing region itself),
   it frees places in the array tabNeighbors. To avoid a shift of the other points in order to have no holes
   in the array, we'd better keep an array of the freed places, so as to use them later when new neighbors
   are added */
struct FlstLinkedNeighbor 
    { 
      struct LsPoint point; /* The point representing the first neighbor pixel */
      struct FlstLinkedNeighbor* pNextPoint; /* The next point of same gray-level */
    };
struct FlstNeighborhood
{
  struct FlstLinkedNeighbor
    *tabNeighbors, **tabpFreePlaces; /* The array of points and of pointers to freed points */
  int iNbPoints, iNbFreePlaces; /* The size of the previous arrays */
  int tabOccupation[256]; /* The number of neighbors at each gray-level */
  struct FlstLinkedNeighbor *tabpFirstNeighbor[256], *tabpLastNeighbor[256]; /* For each gray-level, the last included neighbor */
  unsigned char cMinGrayLevel, cMaxGrayLevel; /* Min and max gray-levels of the neighbors */
};

/* Well, here are global variables. It is not very beautiful, but it avoids to weigh the code too much,
   since they are used almost everywhere */
static int iWidth, iHeight, iMinArea, iMaxArea, iMaxAreaWork, iAreaImage, iHalfAreaImage;
static int iLengthOfBorder;
static int iIndexOfExploration;
static struct LsPoint* tabPointsInCurrentRegion;
static int **visitefront; /* An image indicating the points belonging to the exterior frontier */
static struct FlstFrontierPixel** tabtabFrontierPixels;
/* Gives for each configuration of the frontier around the pixel the number of new connected components
   of the complementary created (sometimes negative, since cc can be deleted) */
static int tabPattern4[256], tabPattern8[256];
/* Gives for each configuration of the frontier around the pixel the number of new frontier pixels */
static int tabLength[256];

static LsTree* pGlobalTree;
static bool bDoesShapeMeetBorder;

/* --------------------------------------------------------------------------------------
   --------- Functions to manage the neighborhood structure -----------------------------
   -------------------------------------------------------------------------------------- */

/* Reinitialise the neighborhood, so that it will be used for a new region */
void flst_reinit_neighborhood(struct FlstNeighborhood* pNeighborhood)
{
  pNeighborhood->iNbPoints = pNeighborhood->iNbFreePlaces = 0;
  memset(pNeighborhood->tabOccupation, 0, 256 * sizeof(int));
  pNeighborhood->cMinGrayLevel = 255; pNeighborhood->cMaxGrayLevel = 0;  
}

/* To allocate the structure representing the neighborhood of a region */
int init_neighborhood(struct FlstNeighborhood* pNeighborhood, int iMaxArea)
{
  iMaxArea = 4*(iMaxArea+1);
  if(iMaxArea > iWidth*iHeight)
    iMaxArea = iWidth*iHeight;

  pNeighborhood->tabNeighbors = new FlstLinkedNeighbor[iMaxArea];
  if(pNeighborhood->tabNeighbors == 0) {
    mwerror(FATAL, 1, "init_neighborhood --> impossible to allocate the array of neighbors\n");
    return 0;
  }
  pNeighborhood->tabpFreePlaces = new FlstLinkedNeighbor*[iMaxArea];
  if(pNeighborhood->tabpFreePlaces == 0) {
    delete [] pNeighborhood->tabNeighbors;
    mwerror(FATAL, 1, "init_neighborhood --> impossible to allocate the array of neighbors\n");
    return 0;
  }
  memset(pNeighborhood->tabpFirstNeighbor, 0, 256*sizeof(struct FlstLinkedNeighbor*));
  memset(pNeighborhood->tabpLastNeighbor, 0, 256*sizeof(struct FlstLinkedNeighbor*));
  flst_reinit_neighborhood(pNeighborhood);
  return 1;
}

/* Free the structure representing the neighborhood of a region */
void free_neighborhood(struct FlstNeighborhood* pNeighborhood)
{
  delete [] pNeighborhood->tabNeighbors;
  delete [] pNeighborhood->tabpFreePlaces;
}

/* Add the pixel (x,y), of gray-level cGrayLevel, to the neighbor pixels */
void flst_add_neighbor(struct FlstNeighborhood* pNeighborhood,
		       short int x, short int y, unsigned char cGrayLevel)
{
  struct FlstLinkedNeighbor* pNewNeighbor;

  visitefront[y][x] = iIndexOfExploration;
  if(cGrayLevel < pNeighborhood->cMinGrayLevel) pNeighborhood->cMinGrayLevel = cGrayLevel;
  if(cGrayLevel > pNeighborhood->cMaxGrayLevel) pNeighborhood->cMaxGrayLevel = cGrayLevel;
  /* If we have holes in the array of points, use one to put the new neighbor, else add one element
     at the end of the array */
  if(pNeighborhood->iNbFreePlaces > 0)
    pNewNeighbor = pNeighborhood->tabpFreePlaces[--pNeighborhood->iNbFreePlaces];
  else
    pNewNeighbor = &pNeighborhood->tabNeighbors[pNeighborhood->iNbPoints];
  ++ pNeighborhood->iNbPoints; /* We have one more point */
  pNewNeighbor->point.x = x; /* Initialise the new neighbor point */
  pNewNeighbor->point.y = y;
  if(pNeighborhood->tabOccupation[cGrayLevel] == 0)
    pNeighborhood->tabpFirstNeighbor[cGrayLevel] = pNewNeighbor;
  else
    pNeighborhood->tabpLastNeighbor[cGrayLevel]->pNextPoint = pNewNeighbor;
  pNeighborhood->tabpLastNeighbor[cGrayLevel] = pNewNeighbor; /* Put the new point at the head */
  ++ pNeighborhood->tabOccupation[cGrayLevel]; /* We have one more pixel at this gray-level */
}

/* Delete the first 'iNbToRemove' neighbor pixels of the given gray-level from the neighborhood
   (because they are incorporated in the region itself) */
void flst_delete_neighbors(struct FlstNeighborhood* pNeighborhood, unsigned char cGrayLevel, int iNbToRemove)
{
  int i;
  struct FlstLinkedNeighbor* pCurrentNeighbor = pNeighborhood->tabpFirstNeighbor[cGrayLevel];
  
  pNeighborhood->iNbPoints -= iNbToRemove;
  pNeighborhood->tabOccupation[cGrayLevel] -= iNbToRemove;
  /* Set the places of the points as free places */
  for(i = iNbToRemove - 1; i >= 0; i--, pCurrentNeighbor = pCurrentNeighbor->pNextPoint)
    pNeighborhood->tabpFreePlaces[pNeighborhood->iNbFreePlaces++] = pCurrentNeighbor;

  if(pNeighborhood->tabOccupation[cGrayLevel] != 0)
    pNeighborhood->tabpFirstNeighbor[cGrayLevel] = pCurrentNeighbor;
  else
    {
      if(cGrayLevel == pNeighborhood->cMinGrayLevel)
	while(pNeighborhood->tabOccupation[pNeighborhood->cMinGrayLevel] == 0)
	  ++ pNeighborhood->cMinGrayLevel;
      if(cGrayLevel == pNeighborhood->cMaxGrayLevel)
	while(pNeighborhood->tabOccupation[pNeighborhood->cMaxGrayLevel] == 0)
	  -- pNeighborhood->cMaxGrayLevel;
    }
}

/* Return the smallest gray-level of the neighbor pixels */
unsigned char flst_smallest_neighbor(struct FlstNeighborhood* pNeighborhood)
{
  return pNeighborhood->cMinGrayLevel;
}

/* Return the largest gray-level of the neighbor pixels */
unsigned char flst_largest_neighbor(struct FlstNeighborhood* pNeighborhood)
{
  return pNeighborhood->cMaxGrayLevel;
}

/* -----------------------------------------------------------------------------
   --------- Allocations of structures used in the algorithm -------------------
   -------------------------------------------------------------------------- */

/* Allocate the image of the tags for the visited pixels and the visited neighbor pixels.
   Do not be afraid about the parameter: it is simply a pointer to a 2-D array representing
   an image (because this image is allocated here) */
int init_image_of_visited_pixels(int*** ptabtabImageOfVisitedPixels)
{
  int i, iAreaImage;
  
  iAreaImage = iWidth*iHeight;
  *ptabtabImageOfVisitedPixels = new int*[iHeight];
  if(*ptabtabImageOfVisitedPixels == 0) {
    mwerror(FATAL, 1, "init_image_of_visited_pixels --> impossible to allocate one array\n");
    return 0;
  }
  visitefront = new int*[iHeight];
  if(visitefront == 0) {
    delete [] *ptabtabImageOfVisitedPixels;
    mwerror(FATAL, 1, "init_image_of_visited_pixels --> impossible to allocate one array\n");
    return 0;
  }
  (*ptabtabImageOfVisitedPixels)[0] = new int[iAreaImage];
  memset((*ptabtabImageOfVisitedPixels)[0], 0, iAreaImage*sizeof(int));
  if((*ptabtabImageOfVisitedPixels)[0] == 0) {
    delete [] *ptabtabImageOfVisitedPixels; delete [] visitefront;
    mwerror(FATAL, 1, "init_image_of_visited_pixels --> impossible to allocate: image of visited pixels\n");
    return 0;
  }
  for(i = 1; i < iHeight; i++)
    (*ptabtabImageOfVisitedPixels)[i] = (*ptabtabImageOfVisitedPixels)[i-1] + iWidth;

  visitefront[0] = new int[iAreaImage];
  memset(visitefront[0], 0, iAreaImage*sizeof(int));
  if(visitefront[0] == 0) {
    delete [] *ptabtabImageOfVisitedPixels; delete [] visitefront; delete [] (*ptabtabImageOfVisitedPixels)[0];
    mwerror(FATAL, 1, "init_image_of_visited_pixels -->impossible to allocate image of visited neighbors\n");
    return 0;
  }
  for(i = 1; i < iHeight; i++)
    visitefront[i] = visitefront[i-1] + iWidth;
  return 1;
}

void free_image_of_visited_pixels(int** tabtabImageOfVisitedPixels)
{
  delete [] tabtabImageOfVisitedPixels[0]; /* Remember that the memory for the image is in fact a 1-D array */
  delete [] tabtabImageOfVisitedPixels; /* This was simply an array of pointers to a location in the 1-D array */

  delete [] visitefront[0];
  delete [] visitefront;
}

int init_output_image(unsigned char *tabPixelsIn, unsigned char ***ptabtabPixelsOutput)
{
  int i;

  *ptabtabPixelsOutput = new unsigned char*[iHeight];
  if(*ptabtabPixelsOutput == 0) {
    mwerror(FATAL, 1, "init_output_image --> impossible to allocate the array\n");
    return 0;
  }
  for(i = 0; i < iHeight; i++)
    (*ptabtabPixelsOutput)[i] = tabPixelsIn + i * iWidth;
  return 1;
}

void free_output_image(unsigned char**tabtabPixelsOutput)
{
  delete [] tabtabPixelsOutput;
}

int init_region(int iMaxArea)
{
  tabPointsInCurrentRegion = new LsPoint[iMaxArea];
  if(tabPointsInCurrentRegion == 0) {
    mwerror(FATAL, 1, "init_region --> impossible to allocate the array\n");
    return 0;
  }
  return 1;
}

void free_region()
{
  delete [] tabPointsInCurrentRegion;
}

int init_frontier_pixels()
{
  int i, iAreaImage;
  struct FlstFrontierPixel* tabBuffer;

  iAreaImage = iWidth*iHeight;
  tabBuffer = new FlstFrontierPixel[iAreaImage];
  memset(tabBuffer, 0, iAreaImage*sizeof(struct FlstFrontierPixel));
  if(tabBuffer == 0) {
    mwerror(FATAL, 1, "init_frontier_pixels --> impossible to allocate the image of frontier pixels\n");
    return 0;
  }
  tabtabFrontierPixels = new FlstFrontierPixel*[iHeight];
  if(tabtabFrontierPixels == 0) {
    delete [] tabBuffer;
    mwerror(FATAL, 1, "init_frontier_pixels --> impossible to allocate the access array of frontier pixels\n");
    return 0;
  }
  for(i = 0; i < iHeight; i++)
    tabtabFrontierPixels[i] = tabBuffer + i * iWidth;
  return 1;
}

void free_frontier_pixels()
{
  delete [] tabtabFrontierPixels[0];
  delete [] tabtabFrontierPixels;
}

void init_patterns()
{
  int i;
  unsigned char c4Neighbors;

  /* This is for the shape in 4-connectedness (and the complementary in 8-connectedness) */
  memset(tabPattern4, 0, 256 * sizeof(int));
  for(i = 0; i <= 255; i++)
    {
      c4Neighbors = i & (UP | LEFT | DOWN | RIGHT);
      if(c4Neighbors == (UP | LEFT | DOWN | RIGHT))
	{
	  tabPattern4[i] = -1;
	  if(i & UPLEFT)
	    ++ tabPattern4[i];
	  if(i & LEFTDOWN)
	    ++ tabPattern4[i];
	  if(i & DOWNRIGHT)
	    ++ tabPattern4[i];
	  if(i & RIGHTUP)
	    ++ tabPattern4[i];
	}
      else if(c4Neighbors == (UP | LEFT | DOWN))
	{
	  if(i & UPLEFT)
	    tabPattern4[i] = 1;
	  if(i & LEFTDOWN)
	    ++ tabPattern4[i];
	}
      else if(c4Neighbors == (LEFT | DOWN | RIGHT))
	{
	  if(i & LEFTDOWN)
	    tabPattern4[i] = 1;
	  if(i & DOWNRIGHT)
	    ++ tabPattern4[i];
	}
      else if(c4Neighbors == (DOWN | RIGHT | UP))
	{
	  if(i & DOWNRIGHT)
	    tabPattern4[i] = 1;
	  if(i & RIGHTUP)
	    ++ tabPattern4[i];
	}
      else if(c4Neighbors == (RIGHT | UP | LEFT))
	{
	  if(i & RIGHTUP)
	    tabPattern4[i] = 1;
	  if(i & UPLEFT)
	    ++ tabPattern4[i];
	}
      else if(c4Neighbors == (UP | DOWN))
	tabPattern4[i] = 1;
      else if(c4Neighbors == (RIGHT | LEFT))
	tabPattern4[i] = 1;
      else if(c4Neighbors == (UP | LEFT) && (i & UPLEFT))
	tabPattern4[i] = 1;
      else if(c4Neighbors == (LEFT | DOWN) && (i & LEFTDOWN))
	tabPattern4[i] = 1;
      else if(c4Neighbors == (DOWN | RIGHT) && (i & DOWNRIGHT))
	tabPattern4[i] = 1;
      else if(c4Neighbors == (RIGHT | UP) && (i & RIGHTUP))
	tabPattern4[i] = 1;
    }

  /* This is for the shape in 8-connectedness (and the complementary in 4-connectedness) */
  memset(tabPattern8, 0, 256 * sizeof(int));
  for(i = 0; i <= 255; i++)
    {
      c4Neighbors = i & (UP | LEFT | DOWN | RIGHT);
      if(c4Neighbors == (UP | LEFT | DOWN | RIGHT))
	tabPattern8[i] = -1;
      else if(c4Neighbors == (UP | DOWN))
	tabPattern8[i] = 1;
      else if(c4Neighbors == (RIGHT | LEFT))
	tabPattern8[i] = 1;
      else if(c4Neighbors == LEFT)
	{
	  if(i & DOWNRIGHT)
	    tabPattern8[i] = 1;
	  if(i & RIGHTUP)
	    ++ tabPattern8[i];
	}
      else if(c4Neighbors == DOWN)
	{
	  if(i & RIGHTUP)
	    tabPattern8[i] = 1;
	  if(i & UPLEFT)
	    ++ tabPattern8[i];
	}
      else if(c4Neighbors == RIGHT)
	{
	  if(i & UPLEFT)
	    tabPattern8[i] = 1;
	  if(i & LEFTDOWN)
	    ++ tabPattern8[i];
	}
      else if(c4Neighbors == UP)
	{
	  if(i & LEFTDOWN)
	    tabPattern8[i] = 1;
	  if(i & DOWNRIGHT)
	    ++ tabPattern8[i];
	}
      else if(c4Neighbors == (UP | LEFT) && (i & DOWNRIGHT))
	tabPattern8[i] = 1;
      else if(c4Neighbors == (LEFT | DOWN) && (i & RIGHTUP))
	tabPattern8[i] = 1;
      else if(c4Neighbors == (DOWN | RIGHT) && (i & UPLEFT))
	tabPattern8[i] = 1;
      else if(c4Neighbors == (RIGHT | UP) && (i & LEFTDOWN))
	tabPattern8[i] = 1;
      else if(c4Neighbors == 0)
	{ /* There are pixels in the shape only in diagonal directions */
	  tabPattern8[i] = -1;
	  if(i & UPLEFT)
	    ++ tabPattern8[i];
	  if(i & LEFTDOWN)
	    ++ tabPattern8[i];
	  if(i & DOWNRIGHT)
	    ++ tabPattern8[i];
	  if(i & RIGHTUP)
	    ++ tabPattern8[i];
	  if(tabPattern8[i] == -1) /* This happens only when it is the first pixel in the shape */
	    tabPattern8[i] = 0;
	}
    }
  for(i = 0; i <= 255; i++) {
    tabLength[i] = 4;
    if(i & UP) tabLength[i] -= 2;
    if(i & LEFT) tabLength[i] -= 2;
    if(i & DOWN) tabLength[i] -= 2;
    if(i & RIGHT) tabLength[i] -= 2;
  }
}

/* -----------------------------------------------------------------------------
   --- The algorithm itself, based on growing regions containing an extremum ---
   -------------------------------------------------------------------------- */

/* Indicates if the pixel at position (x, y) is a local minimum in the image */
char is_local_min(unsigned char** ou, short int x, short int y, char bFlagHeightConnectedness)
{
  unsigned char v;
  char n = 0;

  v = ou[y][x];
  return (x==iWidth-1 || (ou[y][x+1]>v && ++n) || ou[y][x+1]==v) &&
    (x==0 || (ou[y][x-1]>v && ++n) || ou[y][x-1]==v) &&
      (y==iHeight-1 || (ou[y+1][x]>v && ++n) || ou[y+1][x]==v) &&
	(y==0 || (ou[y-1][x]>v && ++n) || ou[y-1][x]==v) &&
	  (bFlagHeightConnectedness == 0 ||
	   ((x==iWidth-1 || y==0 || (ou[y-1][x+1]>v  && ++n) || ou[y-1][x+1]==v) &&
	    (x==iWidth-1 || y==iHeight-1 || (ou[y+1][x+1]>v && ++n) || ou[y+1][x+1]==v) &&
	    (x==0 || y==iHeight-1 || (ou[y+1][x-1]>v && ++n) || ou[y+1][x-1]==v) &&
	    (x==0 || y==0 || (ou[y-1][x-1]>v && ++n) || ou[y-1][x-1]==v)))	&&
	      n != 0;
}

/* Indicates whether the pixel at position (x,y) in the image is a local maximum */
char is_local_max(unsigned char** ou, short int x, short int y, char bFlagHeightConnectedness)
{
  unsigned char v;
  char n = 0;

  v = ou[y][x];
  return (x==iWidth-1 || (ou[y][x+1]<v && ++n) || ou[y][x+1]==v) &&
    (x==0 || (ou[y][x-1]<v && ++n) || ou[y][x-1]==v) &&
      (y==iHeight-1 || (ou[y+1][x]<v && ++n) || ou[y+1][x]==v) &&
	(y==0 || (ou[y-1][x]<v && ++n) || ou[y-1][x]==v) && 
	  (bFlagHeightConnectedness == 0 ||
	   ((x==iWidth-1 || y==0 || (ou[y-1][x+1]<v  && ++n) || ou[y-1][x+1]==v) &&
	    (x==iWidth-1 || y==iHeight-1 || (ou[y+1][x+1]<v && ++n) || ou[y+1][x+1]==v) &&
	    (x==0 || y==iHeight-1 || (ou[y+1][x-1]<v && ++n) || ou[y+1][x-1]==v) &&
	    (x==0 || y==0 || (ou[y-1][x-1]<v && ++n) || ou[y-1][x-1]==v))) &&
	      n != 0;
}

/* Put all the pixels of tabPoints at level cNewGrayLevel in the image tabtabPixelsOutput */
void flst_set_at_level(unsigned char** tabtabPixelsOutput, struct LsPoint* tabPoints, int iNbPoints, unsigned char cNewGrayLevel)
{
  int i;
  for(i = iNbPoints - 1; i >= 0; i--)
    tabtabPixelsOutput[tabPoints[i].y][tabPoints[i].x] = cNewGrayLevel;
}

/* Add the point of coordinates (i, j) in the current connected component of level set. Modify the
   configuration of the frontier suitably and update the number of connected components of the 
   complementary (useful to know the number of holes in the shape).
   Note that the treatment is different according to the connectedness: if the shape is in
   4-connectedness, we must consider the complementary as in 8-connectedness, so that the configuration
   of the frontier along the diagonal neighbors is important. */
void flst_add_point4(short int i, short int j, int* pNbConnectedComponentsOfFrontier)
{
  struct FlstFrontierPixel* pFrontierPixel = tabtabFrontierPixels[i] + j;
  unsigned char cPattern = 0;

  if(bDoesShapeMeetBorder)
    {
      if(i == 0) cPattern |= LEFT;
      if(j == 0) cPattern |= DOWN;
    }
  if(pFrontierPixel->iExploration < iIndexOfExploration)
    {
      pFrontierPixel->iExploration = iIndexOfExploration;
      pFrontierPixel->cDirections = 0;
      if(j != 0) pFrontierPixel->cDirections |= UP;
      if(i != 0) pFrontierPixel->cDirections |= RIGHT;
    }
  else
    {
      cPattern |= pFrontierPixel->cDirections & (LEFT | DOWN);
      if(pFrontierPixel->cDirections & LEFT)
	pFrontierPixel->cDirections -= LEFT;
      else if(i != 0)
	pFrontierPixel->cDirections |= RIGHT;
      if(pFrontierPixel->cDirections & DOWN)
	pFrontierPixel->cDirections -= DOWN;
      else if(j != 0)
	pFrontierPixel->cDirections |= UP;
    }

  if(j == iWidth-1)
    {
      if(bDoesShapeMeetBorder) cPattern |= UP;
    }
  else
    if(pFrontierPixel[1].iExploration < iIndexOfExploration)
      {
	pFrontierPixel[1].iExploration = iIndexOfExploration;
	pFrontierPixel[1].cDirections = DOWN;
      }
    else
      {
	cPattern |= pFrontierPixel[1].cDirections & UP;
	if(pFrontierPixel[1].cDirections & RIGHT)
	  cPattern |= UPLEFT;
	if(pFrontierPixel[1].cDirections & UP)
	  pFrontierPixel[1].cDirections -= UP;
	else
	  pFrontierPixel[1].cDirections |= DOWN;
      }
  
  if(i == iHeight-1)
    {
      if(bDoesShapeMeetBorder) cPattern |= RIGHT;
    }
  else
    if(pFrontierPixel[iWidth].iExploration < iIndexOfExploration)
      {
	pFrontierPixel[iWidth].iExploration = iIndexOfExploration;
	pFrontierPixel[iWidth].cDirections = LEFT;
      }
    else
      {
	cPattern |= pFrontierPixel[iWidth].cDirections & RIGHT;
	if(pFrontierPixel[iWidth].cDirections & UP)
	  cPattern |= DOWNRIGHT;
	if(pFrontierPixel[iWidth].cDirections & RIGHT)
	  pFrontierPixel[iWidth].cDirections -= RIGHT;
	else
	  pFrontierPixel[iWidth].cDirections |= LEFT;
      }

  /* Look if the number of connected components of the frontier is changing */
  if(j > 0 && pFrontierPixel[-1].iExploration == iIndexOfExploration && 
     (pFrontierPixel[-1].cDirections & RIGHT))
    cPattern |= LEFTDOWN;
  if(i < iHeight-1 && j < iWidth-1 && pFrontierPixel[iWidth+1].iExploration == iIndexOfExploration && 
     (pFrontierPixel[iWidth+1].cDirections & DOWN))
    cPattern |= RIGHTUP;
  *pNbConnectedComponentsOfFrontier += tabPattern4[cPattern];
  
  /* Compute the new length of the border */
  if(j == 0) { --iLengthOfBorder; if(cPattern & DOWN) cPattern -= DOWN; }
  else if(j == iWidth-1) { --iLengthOfBorder; if(cPattern & UP) cPattern -= UP; }
  if(i == 0) { --iLengthOfBorder; if(cPattern & LEFT) cPattern -= LEFT; }
  else if(i == iHeight-1) { --iLengthOfBorder; if(cPattern & RIGHT) cPattern -= RIGHT; }
  iLengthOfBorder += tabLength[cPattern];

  if(j == 0 || j == iWidth-1 || i == 0 || i == iHeight-1)
    bDoesShapeMeetBorder = true;
}

/* Add the point of coordinates (i, j) in the current connected component of level set. Modify the
   configuration of the frontier suitably and update the number of connected components of the 
   complementary (useful to know the number of holes in the shape).
   Note that the treatment is different according to the connectedness: if the shape is in
   4-connectedness, we must consider the complementary as in 8-connectedness, so that the configuration
   of the frontier along the diagonal neighbors is important. */
void flst_add_point8(short int i, short int j, int *pNbConnectedComponentsOfFrontier)
{
  struct FlstFrontierPixel* pFrontierPixel = tabtabFrontierPixels[i] + j;
  unsigned char cPattern = 0;

  if(bDoesShapeMeetBorder)
    {
      if(i == 0) cPattern |= LEFT;
      if(j == 0) cPattern |= DOWN;
    }
  if(pFrontierPixel->iExploration < iIndexOfExploration)
    {
      pFrontierPixel->iExploration = iIndexOfExploration;
      pFrontierPixel->cDirections = 0;
      if(j != 0) pFrontierPixel->cDirections |= UP;
      if(i != 0) pFrontierPixel->cDirections |= RIGHT;
    }
  else
    {
      cPattern |= pFrontierPixel->cDirections & (LEFT | DOWN);
      if(pFrontierPixel->cDirections & LEFT)
	pFrontierPixel->cDirections -= LEFT;
      else if(i != 0)
	pFrontierPixel->cDirections |= RIGHT;
      if(pFrontierPixel->cDirections & DOWN)
	pFrontierPixel->cDirections -= DOWN;
      else if(j != 0)
	pFrontierPixel->cDirections |= UP;
    }

  if(j == iWidth-1)
    {
      if(bDoesShapeMeetBorder) cPattern |= UP;
    }
  else
    if(pFrontierPixel[1].iExploration < iIndexOfExploration)
      {
	pFrontierPixel[1].iExploration = iIndexOfExploration;
	pFrontierPixel[1].cDirections = DOWN;
      }
    else
      {
	cPattern |= pFrontierPixel[1].cDirections & UP;
	if(pFrontierPixel[1].cDirections & LEFT)
	  cPattern |= UPLEFT;
	if(pFrontierPixel[1].cDirections & UP)
	  pFrontierPixel[1].cDirections -= UP;
	else
	  pFrontierPixel[1].cDirections |= DOWN;
      }

  if(i == iHeight-1)
    {
      if(bDoesShapeMeetBorder) cPattern |= RIGHT;
    }
  else
    if(pFrontierPixel[iWidth].iExploration < iIndexOfExploration)
      {
	pFrontierPixel[iWidth].iExploration = iIndexOfExploration;
	pFrontierPixel[iWidth].cDirections = LEFT;
      }
    else
      {
	cPattern |= pFrontierPixel[iWidth].cDirections & RIGHT;
	if(pFrontierPixel[iWidth].cDirections & DOWN)
	  cPattern |= DOWNRIGHT;
	if(pFrontierPixel[iWidth].cDirections & RIGHT)
	  pFrontierPixel[iWidth].cDirections -= RIGHT;
	else
	  pFrontierPixel[iWidth].cDirections |= LEFT;
      }
  
  /* Treat the modification of the number of connected components of the frontier */
  if(j > 0 && pFrontierPixel[-1].iExploration == iIndexOfExploration && 
     (pFrontierPixel[-1].cDirections & LEFT))
    cPattern |=  LEFTDOWN;
  if(i < iHeight-1 && j < iWidth-1 && pFrontierPixel[iWidth+1].iExploration == iIndexOfExploration && 
     (pFrontierPixel[iWidth+1].cDirections & UP))
    cPattern |= RIGHTUP;
  *pNbConnectedComponentsOfFrontier += tabPattern8[cPattern];

  /* Compute the new length of the border */
  if(j == 0) { --iLengthOfBorder; if(cPattern & DOWN) cPattern -= DOWN; }
  else if(j == iWidth-1) { --iLengthOfBorder; if(cPattern & UP) cPattern -= UP; }
  if(i == 0) { --iLengthOfBorder; if(cPattern & LEFT) cPattern -= LEFT; }
  else if(i == iHeight-1) { --iLengthOfBorder; if(cPattern & RIGHT) cPattern -= RIGHT; }
  iLengthOfBorder += tabLength[cPattern];

  if(j == 0 || j == iWidth-1 || i == 0 || i == iHeight-1)
    bDoesShapeMeetBorder = true;
}

/// Insert a new shape in the tree.
static void flst_insert_child_in_tree(struct LsShape* parent, struct LsShape* pNewChildToInsert)
{
  pNewChildToInsert->parent = parent;
  pNewChildToInsert->sibling = parent->child;
  parent->child = pNewChildToInsert;
}

void flst_create_new_shape(int iCurrentArea, unsigned char cCurrentGrayLevel,
                           LsShape::Type type)
{
  struct LsShape* pNewShape = &pGlobalTree->shapes[pGlobalTree->iNbShapes++];

  pNewShape->type = type;
  pNewShape->gray = cCurrentGrayLevel;
  pNewShape->bBoundary = bDoesShapeMeetBorder;
  //  pNewShape->iPerimeter = iLengthOfBorder;
  pNewShape->area = iCurrentArea;
  pNewShape->bIgnore = false;
  //  pNewShape->pData = 0;
  /* Insert the new shape in the tree */
  pNewShape->child = 0;
  flst_insert_child_in_tree(&pGlobalTree->shapes[0], pNewShape);
}

static struct LsShape* ls_previous_sibling(struct LsShape* pShape)
{
  struct LsShape* pSibling = pShape->parent->child;
  if(pSibling == pShape) return 0;
  while(pSibling->sibling != pShape)
    pSibling = pSibling->sibling;
  return pSibling;
}

/* Update the smallest shape and the largest shape containing each pixel of tabPoints */
static void flst_update_image_of_indexes(struct LsPoint* tabPoints, int iNbPoints, struct LsShape** tabImageOfLargestShape)
{
  int i, iAbsoluteCoordinate;
  struct LsShape *pNewShape, *pIncludedShape;
  struct LsShape* pRoot = &pGlobalTree->shapes[0];

  pNewShape = &pGlobalTree->shapes[pGlobalTree->iNbShapes-1];
  for(i = iNbPoints - 1; i >= 0; i--)
    {
      iAbsoluteCoordinate = tabPoints[i].y * iWidth + tabPoints[i].x;
      if(tabImageOfLargestShape[iAbsoluteCoordinate] == pRoot)
	pGlobalTree->smallestShape[iAbsoluteCoordinate] = pNewShape;
      else
	{
	  pIncludedShape = tabImageOfLargestShape[iAbsoluteCoordinate];
	  if(pIncludedShape->parent != pNewShape)
	    {
	      /* The previous sibling of pIncludedShape cannot be 0 since the current shape is inserted */
	      ls_previous_sibling(pIncludedShape)->sibling = pIncludedShape->sibling;
	      /* Insert the shape in the list of children of the current shape */
	      flst_insert_child_in_tree(pNewShape, pIncludedShape);
	    }
	}
      tabImageOfLargestShape[iAbsoluteCoordinate] = pNewShape;
    }
}

/* Add the points in the neighborhood of gray level cCurrentGrayLevel to the region tabPointsInCurrentRegion
   and return 1 if we are below the maximum area of a region */
char flst_add_iso_level(struct LsPoint* tabPointsInCurrentRegion, int* pCurrentArea, unsigned char cCurrentGrayLevel, struct FlstNeighborhood* pNeighborhood, unsigned char** ou,
			int** tabtabImageOfVisitedPixels,
			int* pNbConnectedComponentsOfFrontier, char* pFlagHeightConnectedness)
{
  short int x, y;
  struct FlstLinkedNeighbor* pNeighbor;
  int i, iAreaOfIsoLevel = pNeighborhood->tabOccupation[cCurrentGrayLevel], iCurrentArea;

  if(*pCurrentArea + iAreaOfIsoLevel >= iMaxAreaWork)
    return 0;
  if(bDoesShapeMeetBorder && *pCurrentArea + iAreaOfIsoLevel > iHalfAreaImage)
    {
      pGlobalTree->shapes[0].gray = cCurrentGrayLevel;
      //      printf("Gray level of root: %d\n", (int)cCurrentGrayLevel);
      return 0;
    }
  iCurrentArea = *pCurrentArea;
  pNeighbor = pNeighborhood->tabpFirstNeighbor[cCurrentGrayLevel];
  for(i = iAreaOfIsoLevel-1; i >= 0; i--, pNeighbor = pNeighbor->pNextPoint)
    {
      x = pNeighbor->point.x;
      y = pNeighbor->point.y;
      tabPointsInCurrentRegion[iCurrentArea].x = x;
      tabPointsInCurrentRegion[iCurrentArea++].y = y;
      if(*pFlagHeightConnectedness)
	flst_add_point8(y, x, pNbConnectedComponentsOfFrontier);
      else
	flst_add_point4(y, x, pNbConnectedComponentsOfFrontier);
      tabtabImageOfVisitedPixels[y][x] = iIndexOfExploration;
      if(x > 0 && NOTVISITED(x-1,y))
	flst_add_neighbor(pNeighborhood, x-1, y, ou[y][x-1]);
      if(x < iWidth-1 && NOTVISITED(x+1,y))
	flst_add_neighbor(pNeighborhood, x+1, y, ou[y][x+1]);
      if(y > 0 && NOTVISITED(x,y-1))
	flst_add_neighbor(pNeighborhood, x, y-1, ou[y-1][x]);
      if(y < iHeight-1 && NOTVISITED(x,y+1))
	flst_add_neighbor(pNeighborhood, x, y+1, ou[y+1][x]);
      if(flst_smallest_neighbor(pNeighborhood) < cCurrentGrayLevel)
	*pFlagHeightConnectedness = (char)1;
      if(*pFlagHeightConnectedness)
	{      
	  if(x > 0 && y > 0 && NOTVISITED(x-1,y-1))
	    flst_add_neighbor(pNeighborhood, x-1, y-1, ou[y-1][x-1]);
	  if(x < iWidth-1 && y > 0 && NOTVISITED(x+1,y-1))
	    flst_add_neighbor(pNeighborhood, x+1, y-1, ou[y-1][x+1]);
	  if(x < iWidth-1 && y < iHeight-1 && NOTVISITED(x+1,y+1))
	    flst_add_neighbor(pNeighborhood, x+1, y+1, ou[y+1][x+1]);
	  if(x > 0 && y < iHeight-1 && NOTVISITED(x-1,y+1))
	    flst_add_neighbor(pNeighborhood, x-1, y+1, ou[y+1][x-1]);
	}
    }
  *pCurrentArea += iAreaOfIsoLevel;
  flst_delete_neighbors(pNeighborhood, cCurrentGrayLevel, iAreaOfIsoLevel);
  return 1;
}

void flst_find_levels(unsigned char **ou, int** tabtabImageOfVisitedPixels, short int x, short int y,
		      char bFlagHeightConnectedness, struct FlstNeighborhood* pNeighborhood, struct LsShape** tabImageOfLargestShape)
{
  unsigned char cCurrentGrayLevel, cSmallestGrayLevelOfNeighbors, cLargestGrayLevelOfNeighbors;
  int iCurrentArea = 0, iPreviousArea = 0;
  /* The number of connected components of the frontier of the current region (= 1 + number of holes) */
  int iNbConnectedComponentsOfFrontier = 1;
  char bAmbiguityConnectedness = 0;

  bDoesShapeMeetBorder = false; /* The shape does not meet a priori the border of the image */
  iLengthOfBorder = 0;
  cSmallestGrayLevelOfNeighbors = cLargestGrayLevelOfNeighbors = cCurrentGrayLevel = ou[y][x];
  flst_reinit_neighborhood(pNeighborhood); /* No neighbor yet */
  flst_add_neighbor(pNeighborhood, x, y, cCurrentGrayLevel);
  do {
    if(flst_add_iso_level(tabPointsInCurrentRegion, &iCurrentArea, cCurrentGrayLevel, pNeighborhood, ou,
			  tabtabImageOfVisitedPixels,
			  &iNbConnectedComponentsOfFrontier, &bFlagHeightConnectedness) == 0)
      break;
    cSmallestGrayLevelOfNeighbors = flst_smallest_neighbor(pNeighborhood);
    cLargestGrayLevelOfNeighbors = flst_largest_neighbor(pNeighborhood);
    if(bAmbiguityConnectedness && (cSmallestGrayLevelOfNeighbors != cCurrentGrayLevel ||
				   cLargestGrayLevelOfNeighbors != cCurrentGrayLevel)) {
      bAmbiguityConnectedness = 0;
      iNbConnectedComponentsOfFrontier = 1;
    }
    if(cSmallestGrayLevelOfNeighbors > cCurrentGrayLevel ||
       cLargestGrayLevelOfNeighbors < cCurrentGrayLevel)
      {
	if(iNbConnectedComponentsOfFrontier > 1)
	  break;
	iPreviousArea = iCurrentArea;
	if(iMinArea <= iCurrentArea && iCurrentArea <= iMaxArea) {
	  flst_create_new_shape(iCurrentArea, cCurrentGrayLevel,
				(cCurrentGrayLevel < cSmallestGrayLevelOfNeighbors) ? LsShape::INF : LsShape::SUP);
	  flst_update_image_of_indexes(tabPointsInCurrentRegion, iCurrentArea, tabImageOfLargestShape);
	}
	if(cSmallestGrayLevelOfNeighbors > cCurrentGrayLevel)
	  cCurrentGrayLevel = cSmallestGrayLevelOfNeighbors;
	else
	  cCurrentGrayLevel = cLargestGrayLevelOfNeighbors;
	if(cSmallestGrayLevelOfNeighbors == cLargestGrayLevelOfNeighbors) {
	  bFlagHeightConnectedness = 0;
	  bAmbiguityConnectedness = (char)1;
	}
      }
  } while(cSmallestGrayLevelOfNeighbors >= cCurrentGrayLevel ||
	  cLargestGrayLevelOfNeighbors <= cCurrentGrayLevel);
  flst_set_at_level(ou, tabPointsInCurrentRegion, iPreviousArea, cCurrentGrayLevel);
}

void flst_scan_levels(unsigned char **tabtabPixelsOutput, int** tabtabImageOfVisitedPixels, struct FlstNeighborhood* pNeighborhood, struct LsShape** tabImageOfLargestShape)
{
  short int i, j;
  char bFlagHeightConnectedness = 0;

  for(i = 0; i < iHeight; i++)
    for(j = 0; j < iWidth; j++)
      if(tabtabImageOfVisitedPixels[i][j] == 0 &&
	 (is_local_min(tabtabPixelsOutput, j, i, (char)0) ||
	  (is_local_max(tabtabPixelsOutput, j, i, (char)1) && (bFlagHeightConnectedness = 1) != 0)))
	{
	  flst_find_levels(tabtabPixelsOutput, tabtabImageOfVisitedPixels, j, i,
			   bFlagHeightConnectedness, pNeighborhood, tabImageOfLargestShape);
	  bFlagHeightConnectedness = 0;
	  ++ iIndexOfExploration;
	}
}

/* Allocate a new structure for a tree. The true type of the returned pointer is "LsTree". Be careful
   that the structure is allocated, but its pointer fields are not (use mw_ls_change_tree) */
LsTree* ls_new_tree()
{
  LsTree* pNewTree = new LsTree;
  if(pNewTree == 0) {
    mwerror(FATAL, 1, "ls_new_tree --> Impossible to allocate the tree structure\n");
    return NULL;
  }
  pNewTree->ncol = pNewTree->nrow = 0;
  pNewTree->shapes = NULL;
  pNewTree->iNbShapes = 0;
  pNewTree->smallestShape = NULL;
  //  pNewTree->tabData = NULL;
  //  pNewTree->tabpSelectedShapes = NULL;
  //  pNewTree->iNbSelectedShapes = 0;
  return pNewTree;
}

int ls_change_tree(LsTree* pTree, Cimage pCharImageInput)
{
  struct LsShape* pRoot;
  int i;

  /* First delete the previous arrays */
  ls_delete_tree(pTree);

  /* The number of shapes cannot be larger than the number of pixels */
  pTree->ncol = pCharImageInput->ncol; pTree->nrow = pCharImageInput->nrow;
  pRoot = pTree->shapes = new LsShape[pTree->nrow*pTree->ncol];
  if(pTree->shapes == 0) {
    mwerror(FATAL, 1, "ls_change_tree --> impossible to allocate the array of shapes\n");
    return 0;
  }
  /* Set the root of the tree */
  pRoot->type = LsShape::INF; pTree->shapes[0].gray = 255;
  pRoot->bBoundary = true;
  //  pRoot->iPerimeter = 2 * (pCharImageInput->ncol + pCharImageInput->nrow);
  pRoot->bIgnore = false;
  pRoot->area = pCharImageInput->nrow * pCharImageInput->ncol;
  //  pRoot->pData = NULL;
  pRoot->parent = pRoot->sibling = pRoot->child = 0;
  pRoot->pixels = NULL;
  pTree->iNbShapes = 1;

  pTree->smallestShape = new LsShape*[pTree->ncol*pTree->nrow];
  if(pTree->smallestShape == 0) {
    delete [] pTree->shapes; pTree->shapes = NULL;
    mwerror(FATAL, 1, "mw_ls_change_tree --> impossible to allocate the image of smallest shape\n");
    return 0;
  }
  for(i = pTree->ncol*pTree->nrow-1; i >= 0; i--)
    pTree->smallestShape[i] = pRoot;
  return 1;
}

void ls_delete_tree(LsTree* pTree)
{
  if(pTree->shapes != 0 && pTree->iNbShapes > 0)
    delete [] pTree->shapes[0].pixels;
  delete [] pTree->shapes;
  delete [] pTree->smallestShape;
}

/* Reconstruct an image from the tree */
/* __declspec( dllexport )*/
// int ls_tree_to_image(LsTree pTree, Cimage pCharImageOutput)
// {
//   int i;
//   unsigned char *pOutputPixel;
//   struct LsShape** ppShapeOfPixel;
//   struct LsShape* pShape;

//   if(! mw_change_cimage(pCharImageOutput, pTree->nrow, pTree->ncol)) {
//     mwerror(FATAL, 1, "mw_ls_tree_to_image --> impossible to allocate the output image\n");
//     return 0;
//   }
  
//   pOutputPixel = pCharImageOutput->gray;
//   ppShapeOfPixel = pTree->smallestShape;
//   for(i = pTree->nrow*pTree->ncol-1; i >= 0; i--)
//     {
//       pShape = *ppShapeOfPixel++;
//       while(pShape->bRemove)
// 	pShape = pShape->parent;
//       *pOutputPixel++ = pShape->gray;
//     }
//   return 1;
// }

/* Associate to each shape its array of pixels, meaning that we initialize the field
   tabPixels of each shape. The tree structure is used to avoid redundancy in allocated
   memory: each field is in fact a pointer to a subarray of the pixels of the root, which
   are organized in a smart way. The size of the array tabPixels is of course the area of
   the shape */
static int flst_find_pixels_of_shapes2(LsTree* pTree)
{
  struct LsShape **tabpShapesOfStack, *pShape, **ppShape;
  int *tabNbOfProperPixels, *pNbOfProperPixels; /* Indicates for each shape its number of proper pixels */
  int i, j, iSizeOfStack, iIndex;
  struct LsPoint *tabPixelsOfRoot, *pCurrentPoint;

  /* 1) Find for each shape its number of proper pixels, that is pixels contained in the shape but not
     in one of its children. */
  if((tabNbOfProperPixels = new int[pTree->iNbShapes]) == 0) {
    mwerror(FATAL, 1, "flst_find_pixels_of_shapes --> Allocation of the array of proper pixels failed\n");
    return 0;
  }
  /* Initialize by the area */
  pShape = pTree->shapes + pTree->iNbShapes-1;
  pNbOfProperPixels = tabNbOfProperPixels + pTree->iNbShapes-1;
  for(i = pTree->iNbShapes-1; i >= 0; i--)
    *pNbOfProperPixels-- = (pShape--)->area;
  /* For each shape, substract its area to its parent */
  pShape = pTree->shapes + pTree->iNbShapes-1;
  for(i = pTree->iNbShapes-1; i > 0; i--, pShape--)
    tabNbOfProperPixels[pShape->parent - pTree->shapes] -= pShape->area;

  /* 2) Enumerate the shapes in preorder. What follows is equivalent (but more efficient) to
     the following call: unwrap_tree(pTree->shapes[0]) where the recursive function unwrap_tree would
     be: unwrap_tree(pShape) { add_proper_pixels(pShape); for each child of pShape, unwrap_tree(child); }
     This nonrecursive version is similar to [Aho,Hopcroft,Ullman, Data Structures and Algorithms,
     Addison-Wesley, 1983, p. 85]. It uses a temporary stack of shapes (tabpShapesOfStack) giving
     the path from the root to the current shape. Thus its max size is the depth of the tree (not computed),
     therefore always <= to the number of shapes */
  tabpShapesOfStack = new LsShape*[pTree->iNbShapes];
  if(tabpShapesOfStack == 0) {
    delete [] tabNbOfProperPixels;
    mwerror(FATAL, 1, "flst_find_pixels_of_shapes --> Allocation of the stack of shapes failed\n");
    return 0;
  }
  pShape = &pTree->shapes[0];
  tabPixelsOfRoot = pShape->pixels = new LsPoint[pTree->nrow*pTree->ncol];
  if(tabPixelsOfRoot == 0) {
    delete [] tabNbOfProperPixels; delete [] tabpShapesOfStack;
    mwerror(FATAL, 1, "flst_find_pixels_of_shapes --> impossible to allocate the pixels of the root\n");
    return 0;
  }
  iSizeOfStack = 0; i = 0;
  while(1)
    if(pShape != 0) {
      /* Write pixels of pShape */
      pShape->pixels = &tabPixelsOfRoot[i];
      iIndex = (int)(pShape - pTree->shapes);
      i += tabNbOfProperPixels[iIndex];
      tabpShapesOfStack[iSizeOfStack++] = pShape; /* Push the shape in the stack */
      pShape = pShape->child;
    } else {
      if(iSizeOfStack == 0)
	break;
      pShape = tabpShapesOfStack[--iSizeOfStack]->sibling; /* Pop the shape in the stack */
    }
  delete [] tabpShapesOfStack;

  /* 3) Write the pixels */
  ppShape = pTree->smallestShape + pTree->ncol*pTree->nrow-1;
  for(i = pTree->nrow-1; i >= 0; i--)
    for(j = pTree->ncol-1; j >= 0; j--)
      {
	iIndex = (int)((*ppShape) - pTree->shapes);
	pCurrentPoint = &(*ppShape--)->pixels[--tabNbOfProperPixels[iIndex]];
	pCurrentPoint->x = j; pCurrentPoint->y = i;
      }
  delete [] tabNbOfProperPixels;
  return 1;
}

int flst_find_pixels_of_shapes(LsTree* pTree)
{
    std::map<LsShape*, int> areas;
    LsPoint* pts = new LsPoint[pTree->nrow*pTree->ncol];
    if(pts == 0) {
        mwerror(FATAL, 1, "impossible to allocate pixels of root\n");
        return 0;
    }
    int iPixelsReserved = 0;
    for(LsShape* root = pTree->shapes; root; root = root->sibling) {
        LsTreeIterator it, end;
        it = LsTreeIterator(LsTreeIterator::Pre, root);
        end = LsTreeIterator::end(LsTreeIterator::Pre, root);
        assert(it != end);
        for(; it != end; ++it) {
            areas[*it] = (*it)->area;
            if(*it != root)
                areas[(*it)->parent] -= (*it)->area;
        }

        it = LsTreeIterator(LsTreeIterator::Pre, root);
        for(; it != end; ++it) {
            (*it)->pixels = &pts[iPixelsReserved];
            iPixelsReserved += areas[*it];
        }
    }
    assert(iPixelsReserved <= pTree->ncol*pTree->nrow);
    /* 3) Write the pixels */
    LsShape** ppShape=pTree->smallestShape+pTree->ncol*pTree->nrow-1;
    for(int i = pTree->nrow-1; i >= 0; i--)
        for(int j = pTree->ncol-1; j >= 0; j--, ppShape--) {
            std::map<LsShape*, int>::iterator it = areas.find(*ppShape);
            if(it == areas.end())
                continue;
            int index = -- (*it).second;
            LsPoint* pt = &(*ppShape)->pixels[index];
            pt->x = j; pt->y = i;
        }
    return 1;
}

/* Return in the subtree of root pShape a shape that is not removed, 0 if all shapes are removed */
static struct LsShape* ls_shape_of_subtree(struct LsShape* pShape)
{
  struct LsShape* pShapeNotRemoved = 0;
  if(pShape == 0 || ! pShape->bIgnore)
    return pShape;
  for(pShape = pShape->child; pShape != 0; pShape = pShape->sibling)
    if((pShapeNotRemoved = ls_shape_of_subtree(pShape)) != 0)
      break;
  return pShapeNotRemoved;
}

/* To find the true parent, that is the greatest non removed ancestor */
struct LsShape* ls_parent(struct LsShape* pShape)
{
    if(pShape == NULL) {
      mwerror(FATAL, 1, "ls_parent --> null shape\n");
      return NULL;
    }
    do
        pShape = pShape->parent;
    while(pShape && pShape->bIgnore);
    return pShape;
}

/// Find the first child, taking into account that some shapes are removed
struct LsShape* ls_child(struct LsShape* pShape)
{
  struct LsShape* pShapeNotRemoved = 0;
  if(pShape == 0)
    {
      mwerror(FATAL, 1, "ls_child --> null shape\n");
      return(NULL);
    }
  for(pShape = pShape->child; pShape != 0; pShape = pShape->sibling)
    if((pShapeNotRemoved = ls_shape_of_subtree(pShape)) != 0)
      break;
  return pShapeNotRemoved;  
}

/// Find next sibling, taking into account that some shapes are removed
struct LsShape* ls_sibling(struct LsShape* pShape)
{
  struct LsShape *pShape1 = 0, *pShape2 = 0;
  if(pShape == 0)
    {
      mwerror(FATAL, 1, "ls_sibling --> null shape\n");
      return(NULL);
    }
  /* First look at the siblings in the original tree */
  for(pShape1 = pShape->sibling; pShape1 != 0; pShape1 = pShape1->sibling)
    if((pShape2 = ls_shape_of_subtree(pShape1)) != 0)
      return pShape2;
  if(pShape->parent == 0 || ! pShape->parent->bIgnore)
    return 0; /* The parent in the original tree is also the parent in the true tree, nothing more to do */
  /* If not found, find the node in the original tree just before the true parent */
  do
    pShape = pShape->parent;
  while(pShape->parent->bIgnore);
  /* Look at the siblings of this node */
  for(pShape1 = pShape->sibling; pShape1 != 0; pShape1 = pShape1->sibling)
    if((pShape2 = ls_shape_of_subtree(pShape1)) != 0)
      return pShape2;
  return 0;
}

struct LsShape* ls_prev_sibling(struct LsShape* pShape)
{
    LsShape* pNext = ls_parent(pShape);
    if(! pNext)
        return 0;
    pNext = ls_child(pNext);
    LsShape* s = 0;
    while(pNext != pShape) {
        s = pNext;
        pNext = ls_sibling(s);
    }
    return s;
}

/// Smallest non-removed shape at pixel (\a x,\a y).
struct LsShape* ls_smallest_shape(LsTree* pTree, int x, int y)
{
    LsShape* pShape = pTree->smallestShape[y*pTree->ncol + x];
    if(pShape->bIgnore)
        pShape = ls_parent(pShape);
    return pShape;
}

/* --------------------------------------------------------------------------------------
   --------- The main function ----------------------------------------------------------
   -------------------------------------------------------------------------------------- */

#define EXIT_OK 100

/* Function used to exit from the fllt. It frees used memory. There is a levelexit indicating
   at which point we exit the fllt (whether following an allocation error or at normal exit).
   For normal exit, the levelexit must be maximum (we must free all) */
static void exiting_fllt(int levelexit, struct LsShape** tabImageOfLargestShape,
			 int **tabtabImageOfVisitedPixels,
			 struct FlstNeighborhood *pneighborhood,
			 unsigned char **tabtabPixelsOutput)
{
  if(tabImageOfLargestShape != 0) {
    if(levelexit != EXIT_OK)
      ls_delete_tree(pGlobalTree);
    delete [] tabImageOfLargestShape;
  }
  if(levelexit > 1) free_output_image(tabtabPixelsOutput);
  if(levelexit > 2) free_image_of_visited_pixels(tabtabImageOfVisitedPixels);
  if(levelexit > 3) free_neighborhood(pneighborhood);
  if(levelexit > 4) free_region();
  if(levelexit > 5) free_frontier_pixels();
}

/* The "Fast Level Lines Transform" gives the tree of interiors of level lines (named 'shapes')
   representing the image.
   Only shapes of area >= *pMinArea and <= *pMaxArea are put in the tree. If pMinArea==NULL, it
   is interpreted as 1, if pMaxArea==NULL it is understood as the size of the image.
   Notice that if *pMinArea > *pMaxArea, then no shape is extracted, and it is allowed (but
   only in this case) that pTree==NULL.
   Output:
   - the structure pointed to by pTree is filled (pTree must point to an allocated tree).
   - pCharImageInput is modified, where shapes of area < MAX(*pMinArea,*pMaxArea) are removed.
   Notice that this image is useless at output, except when *pMaxArea < *pMinArea, in which case
   it is the grain filtered image. */
int fllt(int* pMinArea, int* pMaxArea, Cimage pCharImageInput, LsTree* pTree)
{
  unsigned char **tabtabPixelsOutput; /* A 2-D array accessing the pixels of the output image */
  struct FlstNeighborhood neighborhood; /* The neighborhood of the current region */
  int** tabtabImageOfVisitedPixels; /* The image saying for each pixel when it has been last visited */
  struct LsShape** tabImageOfLargestShape = 0;
  int i;

  pGlobalTree = pTree;
  iWidth = pCharImageInput->ncol;
  iHeight = pCharImageInput->nrow;

  iAreaImage = iWidth * iHeight; iHalfAreaImage = iAreaImage / 2;
  iMinArea = (pMinArea != NULL && *pMinArea > 0) ? *pMinArea : 1;
  if(iMinArea > iWidth*iHeight) {
    mwerror(USAGE, 1, "flst --> the min area is bigger than the image itself");
    return 0;
  }
  iMaxArea = (pMaxArea != NULL && *pMaxArea > 0) ? *pMaxArea : iWidth*iHeight;
  iMaxAreaWork = (iMinArea < iMaxArea) ? iMaxArea : iMinArea;

  if(iMinArea <= iMaxArea)
    {
      if(! ls_change_tree(pTree, pCharImageInput))
	return 0;
      tabImageOfLargestShape = new LsShape*[iAreaImage];
      if(tabImageOfLargestShape == 0) {
	ls_delete_tree(pTree);
	mwerror(FATAL, 1, "flst --> impossible to allocate the image of indexes of largest shape\n");
	return 0;
      }
      for(i = iAreaImage-1; i >= 0; i--)
	tabImageOfLargestShape[i] = pTree->shapes;
    }

  if(! init_output_image(pCharImageInput->gray, &tabtabPixelsOutput)) {
    exiting_fllt(1,tabImageOfLargestShape, NULL, &neighborhood,tabtabPixelsOutput);
    return 0;
  }
  if(! init_image_of_visited_pixels(&tabtabImageOfVisitedPixels)) {
    exiting_fllt(2,tabImageOfLargestShape,tabtabImageOfVisitedPixels,&neighborhood,tabtabPixelsOutput);
    return 0;
  }
  if(! init_neighborhood(&neighborhood, iMaxAreaWork)) {
    exiting_fllt(3,tabImageOfLargestShape,tabtabImageOfVisitedPixels,&neighborhood,tabtabPixelsOutput);
    return(0);
  }
  if(! init_region(iMaxAreaWork)) {
    exiting_fllt(4,tabImageOfLargestShape,tabtabImageOfVisitedPixels,&neighborhood,tabtabPixelsOutput);
    return 0;
  }

  iIndexOfExploration = 1;

  if(! init_frontier_pixels()) {
    exiting_fllt(5,tabImageOfLargestShape,tabtabImageOfVisitedPixels,&neighborhood,tabtabPixelsOutput);
    return 0;
  }
  init_patterns();

  flst_scan_levels(tabtabPixelsOutput, tabtabImageOfVisitedPixels, &neighborhood,
		   tabImageOfLargestShape);

  if(iMinArea <= iMaxArea && ! flst_find_pixels_of_shapes(pTree)) {
    exiting_fllt(6,tabImageOfLargestShape,tabtabImageOfVisitedPixels,&neighborhood,tabtabPixelsOutput);
    return 0;
  }
  LsPoint* pts = pTree->shapes[0].pixels;
  flst_find_pixels_of_shapes2(pTree);
  assert( memcmp(pTree->shapes[0].pixels, pts,
                 pTree->ncol*pTree->nrow*sizeof(LsPoint)) == 0);
  delete [] pts;
  exiting_fllt(EXIT_OK, tabImageOfLargestShape,tabtabImageOfVisitedPixels,&neighborhood,tabtabPixelsOutput);
  return 1;
}
