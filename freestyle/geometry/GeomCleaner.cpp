
//
//  Copyright (C) : Please refer to the COPYRIGHT file distributed 
//   with this source distribution. 
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
///////////////////////////////////////////////////////////////////////////////

//#if defined(__GNUC__) && (__GNUC__ >= 3)
//// hash_map is not part of the C++ standard anymore;
//// hash_map.h has been kept though for backward compatibility
//# include <hash_map.h>
//#else
//# include <hash_map>
//#endif

#include <stdio.h>
#include <list>
#include <map>
#include "../system/TimeUtils.h"
#include "GeomCleaner.h"

using namespace std;


void GeomCleaner::SortIndexedVertexArray( const float *iVertices, unsigned iVSize, 
                                          const unsigned *iIndices, unsigned iISize, 
                                          real **oVertices, 
                                          unsigned **oIndices)
{
  // First, we build a list of IndexVertex:
  list<IndexedVertex> indexedVertices;
  unsigned i;
  for(i=0; i<iVSize; i+= 3)
  {
    indexedVertices.push_back(IndexedVertex(Vec3r(iVertices[i], iVertices[i+1], iVertices[i+2]), i/3));
  }

  // q-sort
  indexedVertices.sort();

  // build the indices mapping array:
  unsigned *mapIndices = new unsigned[iVSize/3];
  *oVertices = new real[iVSize];
  list<IndexedVertex>::iterator iv;
  unsigned newIndex = 0;
  unsigned vIndex = 0;
  for(iv=indexedVertices.begin(); iv!=indexedVertices.end(); iv++)
  {
    // Build the final results:
    (*oVertices)[vIndex] = iv->x();
    (*oVertices)[vIndex+1] = iv->y();
    (*oVertices)[vIndex+2] = iv->z();

    mapIndices[iv->index()] = newIndex;
    newIndex++;
    vIndex+=3;
  }

  
  // Build the final index array:
  *oIndices  = new unsigned[iISize];
  for(i=0; i<iISize; i++)
  {
    (*oIndices)[i] = 3*mapIndices[iIndices[i]/3];
  }

  delete [] mapIndices;
}

void GeomCleaner::CompressIndexedVertexArray(const real *iVertices, unsigned iVSize, 
                                             const unsigned *iIndices, unsigned iISize, 
                                             real **oVertices, unsigned *oVSize, 
                                             unsigned **oIndices)
{
  // First, we build a list of IndexVertex:
  vector<Vec3r> vertices;
  unsigned i;
  for(i=0; i<iVSize; i+= 3)
  {
    vertices.push_back(Vec3r(iVertices[i], iVertices[i+1], iVertices[i+2]));
  }

  unsigned *mapVertex = new unsigned[iVSize];
  vector<Vec3r>::iterator v = vertices.begin();
  
  vector<Vec3r> compressedVertices;
  Vec3r previous = *v;
  mapVertex[0] = 0;
  compressedVertices.push_back(vertices.front());

  v++;
  Vec3r current;
  i=1;
  for(; v!=vertices.end(); v++)
  {
    current = *v;
    if(current == previous)
      mapVertex[i] = compressedVertices.size()-1;
    else
    {
      compressedVertices.push_back(current);
      mapVertex[i] = compressedVertices.size()-1;
    }  
    previous = current;
    i++;
  }

  // Builds the resulting vertex array:
  *oVSize = 3*compressedVertices.size();
  *oVertices = new real [*oVSize];
  i=0;
  for(v=compressedVertices.begin(); v!=compressedVertices.end(); v++)
  {
    (*oVertices)[i] = (*v)[0];
    (*oVertices)[i+1] = (*v)[1];
    (*oVertices)[i+2] = (*v)[2];
    i += 3;
  }

  // Map the index array:
  *oIndices = new unsigned[iISize];
  for(i=0; i<iISize; i++)
  {
    (*oIndices)[i] = 3*mapVertex[iIndices[i]/3];
  }

  delete [] mapVertex;
}

void GeomCleaner::SortAndCompressIndexedVertexArray(const float *iVertices, unsigned iVSize, 
                                                    const unsigned *iIndices, unsigned iISize, 
                                                    real **oVertices, unsigned *oVSize, 
                                                    unsigned **oIndices)
{

  // tmp arrays used to store the sorted data:
  real *tmpVertices;
  unsigned   *tmpIndices;

  Chronometer chrono;
  // Sort data
  chrono.start();
  GeomCleaner::SortIndexedVertexArray(iVertices, iVSize, 
                                      iIndices, iISize, 
                                      &tmpVertices, &tmpIndices
                                      );
  printf("Sorting: %lf\n", chrono.stop());

  // compress data
  chrono.start();
  GeomCleaner::CompressIndexedVertexArray(tmpVertices, iVSize, 
                                          tmpIndices, iISize, 
                                          oVertices, oVSize,
                                          oIndices);
  printf("Merging: %lf\n", chrono.stop());

  // deallocates memory:
  delete [] tmpVertices;
  delete [] tmpIndices;
}

/*! Defines a hash table used for searching the Cells */
struct GeomCleanerHasher{
#define _MUL 950706376UL
#define _MOD 2147483647UL
  inline size_t operator() (const Vec3r& p) const {
    size_t res = ((unsigned long) (p[0] * _MUL)) % _MOD;
    res = ((res + (unsigned long) (p[1]) * _MUL)) % _MOD;
    return ((res +(unsigned long) (p[2]) * _MUL)) % _MOD;
  }
};

void GeomCleaner::CleanIndexedVertexArray(const float *iVertices, unsigned iVSize, 
					  const unsigned *iIndices, unsigned iISize, 
					  real **oVertices, unsigned *oVSize, 
					  unsigned **oIndices)
{
  typedef map<Vec3r, unsigned> cleanHashTable;
  vector<Vec3r> vertices;
  unsigned i;
  for(i=0; i<iVSize; i+= 3)
    vertices.push_back(Vec3r(iVertices[i], iVertices[i+1], iVertices[i+2]));
  
  cleanHashTable ht;
  vector<unsigned> newIndices;
  vector<Vec3r> newVertices;
  
  // elimination of needless points
  unsigned currentIndex = 0;
  vector<Vec3r>::const_iterator v = vertices.begin();
  vector<Vec3r>::const_iterator end = vertices.end();
  cleanHashTable::const_iterator found;
  for(; v!=end; v++)
  {
    found = ht.find(*v);
    if(found != ht.end())
    {
      // The vertex is already in the new array.
      newIndices.push_back((*found).second);
    }
    else
    {
      newVertices.push_back(*v);
      newIndices.push_back(currentIndex);
      ht[*v] = currentIndex;
      currentIndex++;
    }
  }

  // creation of oVertices array:
  *oVSize = 3*newVertices.size();
  *oVertices = new real[*oVSize];
  currentIndex = 0;
  end = newVertices.end();
  for(v=newVertices.begin(); v!=end ; v++)
  {
    (*oVertices)[currentIndex++] = (*v)[0];
    (*oVertices)[currentIndex++] = (*v)[1];
    (*oVertices)[currentIndex++] = (*v)[2];
  }

  // map new indices:
  *oIndices = new unsigned[iISize];
  for(i=0; i<iISize; i++)
    (*oIndices)[i] = 3*newIndices[iIndices[i]/3];
}
