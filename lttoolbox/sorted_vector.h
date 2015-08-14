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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _SORTEDVECTOR_
#define _SORTEDVECTOR_

class MatchNode;

/**
 * Class representing the sorted vector of destinations for a given
 * MatchNode.  Destinations are also MatchNode pointers.
 */
class SortedVector
{
private:

  /**
   * Pair tag-destination
   */
  struct SVNode
  {
    int tag;
    MatchNode *dest;
  };
  
  /**
   * Array of sorted SVNodes
   */
  SVNode *sv;
  
  /**
   * Size of the array
   */
  int size;
  
  void copy(SortedVector const &o);
  void destroy();
public:
  /**
   * Constructor
   * @param fixed_size size of the SortedVector
   */
  SortedVector(int const fixed_size);
  
  /**
   * Destructor
   */
  ~SortedVector();
  
  /**
   * Copy constructor
   * @param o the item to be copied
   */
  SortedVector(SortedVector const &o);
  
  /**
   * Assignment operator
   * @param o the item to be assigned
   */
  SortedVector & operator =(SortedVector const &o);
  
  /**
   * Method to adding an item into a specified position in the array
   * @param tag the tag of the item
   * @param the destination MatchNode of the item
   * @param pos the position to do the insertion
   */
  void add(int tag, MatchNode *dest, int pos);
  
  /**
   * Searching method (classic binary search)
   * @param tag to search
   * @returns the destination MatchNode pointer
   */ 
  MatchNode * search(int tag);
};

#endif
