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
#ifndef _GENERIC_POOL_
#define _GENERIC_POOL_

#include <list>

using namespace std;

/**
 * Pool of T objects
 */
template <class T>
class Pool
{
private:
  /**
   * Free pointers to objects
   */
  list<T *> free;
  
  /**
   * Currently created objects
   */
  list<T> created;
  
  /**
   * copy method
   * @param other pool object
   */
  void copy(Pool const &p)
  {
    created = p.created;
  
    // all new members are available
    for(typename list<T>::iterator it = created.begin(), limit = created.end();
        it != limit; it++)
    {
      free.push_back(&(*it));
    }
  }
  
  /**
   * destroy method
   */
  void destroy()
  {
    // do nothing
  }
  
  /**
   * Allocate a pool of nelems size
   * @param nelems initial size of the pool
   */
  void init(unsigned int const nelems)
  {
    created.clear();
    free.clear();
    T tmp;
    for(unsigned int i = 0; i != nelems; i++)
    {
      created.push_front(tmp);
      free.push_front(&(*(created.begin())));
    }
  }

  /**
   * Allocate a pool of nelems size with objects equal to 'object'
   * @param nelems initial size of the pool
   * @param object initial value of the objects in the pool
   */
  void init(unsigned int const nelems, T const &object)
  {
    created.clear();
    free.clear();
    for(unsigned int i = 0; i != nelems; i++)
    {
      created.push_front(object);
      free.push_front(&(*(created.begin())));
    }
  }

  
public:
  
  /**
   * Constructor
   */
  Pool()
  {
    init(1);
  }
    
  /**
   * Parametrized constructor
   * @param nelems initial size of the pool
   * @param object initial value of the objects in the pool
   */
  Pool(unsigned int const nelems, T const &object)
  {
    init(nelems, object);
  }
  
  /**
   * Parametrized constructor
   * @param nelems initial size of the pool
   */
  Pool(unsigned int const nelems)
  {
    init(nelems);
  }
  
  /**
   * Destructor
   */
  ~Pool()
  {
    destroy();
  }
  
  /**
   * Copy constructor
   */
  Pool(Pool const &p)
  {
    copy(p);
  }
   
  /**
   * Allocate a pointer to a free 'new' object.
   * @return pointer to the object
   */
  T * get()
  {
    if(free.size() != 0)
    {
      T *result = *(free.begin());
      free.erase(free.begin());
      return result;
    }
    else
    {
      T tmp;
      created.push_front(tmp);
      return &(*(created.begin()));
    }
  }  
  
  /**
   * Release a no more needed instance of a pooled object
   * @param item the no more needed instance of the object
   */ 
  void release(T *item)
  {
    free.push_front(item);
  }
};

#endif
