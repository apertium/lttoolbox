/*
 * Copyright (C) 2005 Universitat d'Alacant / Universidad de Alicante
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */
#include <lttoolbox/sorted_vector.h>
#include <cstdlib>

using namespace std;

void
SortedVector::copy(SortedVector const &o)
{
  sv = new SVNode[o.size];
  size = o.size;
  
  for(int i = 0; i != size; i++)
  {
    sv[i].tag = o.sv[i].tag;
    sv[i].dest = o.sv[i].dest;
  }
}

void
SortedVector::destroy()
{
  delete[] sv;
}

SortedVector::SortedVector(int const fixed_size)
{
  sv = new SVNode[fixed_size];
  size = fixed_size;
}

SortedVector::~SortedVector()
{
  destroy();
}

SortedVector::SortedVector(SortedVector const &o)
{
  copy(o);
}

SortedVector &
SortedVector::operator =(SortedVector const &o)
{
  if(this != &o)
  {
    destroy();
    copy(o);
  }
  return *this;
}

void
SortedVector::add(int tag, MatchNode *dest, int pos)
{
  sv[pos].tag = tag;
  sv[pos].dest = dest; 
}

MatchNode *
SortedVector::search(int tag)
{
  int left = 0, right = size-1;
  while(left <= right)
  {
    int mid = (left+right)/2;
    if(sv[mid].tag == tag)
    {  
      return sv[mid].dest;
    }
    if(sv[mid].tag > tag)
    {
      right = mid - 1;
    }
    else
    {
      left = mid + 1;
    }
  }
  
  return NULL;
}
